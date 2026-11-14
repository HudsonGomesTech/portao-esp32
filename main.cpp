// Portão com ESP32 — Hudson Gomes Tech
// Programa de demonstração, criado apenas para laboratório e estudo. Não foi feito para uso produtivo.
// Placa: ESP32-C3 Super Mini. Só usa as bibliotecas que já vêm com o Arduino para ESP32.
//
// O que faz:
//   1. Sem configuração, abre a rede Wi-Fi "HGT-Portao-XXXX". No celular, entre nela e abra http://192.168.4.1
//      para escolher o Wi-Fi da casa (2,4 GHz) e criar o usuário e a senha da página.
//   2. Configurada, entra no Wi-Fi e serve uma página com login: situação da placa e o botão que dá um toque no relé.
//   3. Segurar o botão BOOT por 5 segundos apaga a configuração e volta ao passo 1.
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include "mbedtls/sha256.h"
#include "esp_random.h"
#include "esp_timer.h"

#define VERSAO "1.0.1"
#define PINO_BOOT 9            // botão BOOT da placa
#define PINO_LED 8             // LED azul da placa (acende em nível baixo)
#define INTERVALO_TOQUES_MS 2000
#define SESSAO_S 43200         // 12 horas
#define MAX_SESSOES 4

struct Config {
  bool ok = false;
  String ssid, senhaWifi, usuario, sal, hash, nome;
  int pino = 5;                // GPIO ligado ao IN do relé
  bool ativoAlto = true;       // o relé liga com nível alto (true) ou baixo (false)
  int pulsoMs = 500;           // duração do toque
} cfg;

struct Sessao { String token; uint32_t ate = 0; } sessoes[MAX_SESSOES];

Preferences prefs;
WebServer web(80);
DNSServer dns;
bool modoConfig = false;
String nomeAp, redesHtml;
uint32_t pulsoAte = 0, ultimoToqueMs = 0, toques = 0, proximaChecagem = 0, bootDesde = 0, bloqueadoAte = 0;
int ultimoToqueS = -1, falhasLogin = 0;
bool pulsando = false, internetOk = false, internetChecada = false;

const char RODAPE[] PROGMEM = "<footer><b>&gt;_ Hudson Gomes Tech</b><br>Programa de demonstra&ccedil;&atilde;o, criado apenas para "
  "laborat&oacute;rio e estudo. N&atilde;o foi feito para uso produtivo. Use por sua conta e risco. Vers&atilde;o " VERSAO "</footer>";
const char ESTILO[] PROGMEM = "<style>*{box-sizing:border-box;margin:0;padding:0}body{font-family:system-ui,Arial,sans-serif;background:#0A0F1A;"
  "color:#F8FAFC;padding:20px 16px;max-width:520px;margin:0 auto}h1{font-size:24px;margin:6px 0 4px}h1 em{color:#22D3EE;font-style:normal}"
  "p.sub{color:#94A3B8;margin-bottom:18px}label{display:block;margin:14px 0 6px;color:#CBD5E1;font-size:14px}"
  "input,select{width:100%;padding:12px;border-radius:10px;border:1px solid #334155;background:#0F172A;color:#F8FAFC;font-size:16px}"
  "button{width:100%;margin-top:20px;padding:16px;border:0;border-radius:12px;background:#22D3EE;color:#0A0F1A;font-size:18px;font-weight:700}"
  "button.sec{background:#1E293B;color:#CBD5E1;font-size:15px;padding:12px}button:disabled{opacity:.5}"
  ".cx{background:#0F172A;border:1px solid #334155;border-radius:14px;padding:14px 16px;margin:14px 0}"
  ".ln{display:flex;justify-content:space-between;padding:7px 0;border-bottom:1px solid #1E293B}.ln:last-child{border:0}.ln span{color:#94A3B8}"
  ".ok{color:#34D399}.er{color:#F87171}.av{background:#422006;border:1px solid #F59E0B;color:#FDE68A;border-radius:12px;padding:12px;margin:14px 0;font-size:14px}"
  "#abrir{font-size:24px;padding:26px}details{margin-top:16px;color:#94A3B8}summary{padding:8px 0}"
  "footer{margin-top:28px;font-size:12px;line-height:1.5;color:#64748B;text-align:center}footer b{color:#22D3EE}</style>";

// ---------- utilidades
uint32_t agoraS() { return (uint32_t)(esp_timer_get_time() / 1000000ULL); }

String esc(const String& s) {
  String o; o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += F("&amp;"); else if (c == '<') o += F("&lt;"); else if (c == '>') o += F("&gt;");
    else if (c == '"') o += F("&quot;"); else if (c == '\'') o += F("&#39;"); else o += c;
  }
  return o;
}

String aleatorio(int bytes) {
  String s; char b[3];
  for (int i = 0; i < bytes; i++) { snprintf(b, sizeof b, "%02x", (uint8_t)(esp_random() & 0xFF)); s += b; }
  return s;
}

String sha256hex(const String& s) {
  uint8_t h[32]; char b[65];
  mbedtls_sha256_context c; mbedtls_sha256_init(&c);
  mbedtls_sha256_starts(&c, 0); mbedtls_sha256_update(&c, (const unsigned char*)s.c_str(), s.length()); mbedtls_sha256_finish(&c, h);
  mbedtls_sha256_free(&c);
  for (int i = 0; i < 32; i++) snprintf(b + 2 * i, 3, "%02x", h[i]);
  return String(b);
}

bool igual(const String& a, const String& b) {            // comparação em tempo constante
  if (a.length() != b.length()) return false;
  uint8_t d = 0; for (size_t i = 0; i < a.length(); i++) d |= a[i] ^ b[i];
  return d == 0;
}

bool pinoValido(int p) { return (p >= 0 && p <= 7) || p == 10 || p == 20 || p == 21; }   // 8 = LED e 9 = BOOT ficam de fora

String pagina(const String& titulo, const String& corpo) {
  String h = F("<!doctype html><html lang=\"pt-BR\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>");
  h += titulo; h += F("</title>"); h += FPSTR(ESTILO); h += F("</head><body><h1><em>&gt;_</em> Port&atilde;o com ESP32</h1>");
  h += corpo; h += FPSTR(RODAPE); h += F("</body></html>");
  return h;
}

void envia(int codigo, const String& html) {
  web.sendHeader(F("Cache-Control"), F("no-store"));
  web.sendHeader(F("X-Content-Type-Options"), F("nosniff"));
  web.sendHeader(F("X-Frame-Options"), F("DENY"));
  web.send(codigo, F("text/html; charset=utf-8"), html);
}

// ---------- configuração guardada na placa
void carregaConfig() {
  prefs.begin("hgt", false);   // leitura e escrita: cria a área na primeira partida, sem erro no log
  cfg.ok = prefs.getBool("ok", false);
  cfg.ssid = prefs.getString("ssid", ""); cfg.senhaWifi = prefs.getString("wifi", "");
  cfg.usuario = prefs.getString("usuario", ""); cfg.sal = prefs.getString("sal", ""); cfg.hash = prefs.getString("hash", "");
  cfg.nome = prefs.getString("nome", "Portao");
  cfg.pino = prefs.getInt("pino", 5); cfg.ativoAlto = prefs.getBool("alto", true); cfg.pulsoMs = prefs.getInt("pulso", 500);
  prefs.end();
  if (!pinoValido(cfg.pino) || cfg.ssid.isEmpty() || cfg.hash.length() != 64) cfg.ok = false;
}

void apagaConfig() { prefs.begin("hgt", false); prefs.clear(); prefs.end(); }

// ---------- relé
void releDesligado() { digitalWrite(cfg.pino, cfg.ativoAlto ? LOW : HIGH); }
void toque() {
  digitalWrite(cfg.pino, cfg.ativoAlto ? HIGH : LOW);
  pulsando = true; pulsoAte = millis() + cfg.pulsoMs; ultimoToqueMs = millis(); ultimoToqueS = agoraS(); toques++;
  Serial.println(F("[rele] toque"));
}

// ---------- modo de configuração (rede própria da placa)
void buscaRedes() {
  redesHtml = "";
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 20; i++) {
    String s = WiFi.SSID(i); if (s.isEmpty()) continue;
    redesHtml += "<option value=\"" + esc(s) + "\">" + esc(s) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  WiFi.scanDelete();
}

void paginaConfig(const String& erro = "") {
  String c = F("<p class=\"sub\">Primeira configura&ccedil;&atilde;o da placa</p>");
  if (erro.length()) c += "<div class=\"av\">" + erro + "</div>";
  c += F("<form method=\"post\" action=\"/salvar\"><div class=\"cx\"><b>1. Wi-Fi da casa</b><label>Rede (use uma de 2,4 GHz)</label><select name=\"ssid\">");
  c += redesHtml;
  c += F("</select><label>Senha do Wi-Fi</label><input type=\"password\" name=\"wifi\" maxlength=\"63\" autocomplete=\"off\"></div>"
         "<div class=\"cx\"><b>2. Acesso &agrave; p&aacute;gina</b><label>Usu&aacute;rio (3 a 32 letras ou n&uacute;meros)</label>"
         "<input name=\"usuario\" maxlength=\"32\" autocapitalize=\"none\" autocomplete=\"off\" required>"
         "<label>Senha (8 caracteres ou mais)</label><input type=\"password\" name=\"senha\" minlength=\"8\" maxlength=\"64\" autocomplete=\"new-password\" required>"
         "<label>Repita a senha</label><input type=\"password\" name=\"senha2\" minlength=\"8\" maxlength=\"64\" autocomplete=\"new-password\" required></div>"
         "<div class=\"cx\"><b>3. Rel&eacute;</b><label>Nome que aparece na p&aacute;gina</label><input name=\"nome\" maxlength=\"24\" value=\"Port&atilde;o\">"
         "<details><summary>Op&ccedil;&otilde;es avan&ccedil;adas</summary><label>Pino (GPIO) ligado ao IN do rel&eacute;</label>"
         "<select name=\"pino\"><option>0</option><option>1</option><option>2</option><option>3</option><option>4</option><option selected>5</option>"
         "<option>6</option><option>7</option><option>10</option><option>20</option><option>21</option></select>"
         "<label>O rel&eacute; liga com</label><select name=\"alto\"><option value=\"1\">n&iacute;vel alto (3,3 V)</option><option value=\"0\">n&iacute;vel baixo (0 V)</option></select>"
         "<label>Dura&ccedil;&atilde;o do toque, em mil&eacute;simos de segundo</label><input name=\"pulso\" type=\"number\" min=\"100\" max=\"3000\" value=\"500\"></details></div>"
         "<button type=\"submit\">Salvar e conectar</button></form>"
         "<form method=\"get\" action=\"/buscar\"><button class=\"sec\" type=\"submit\">Procurar redes de novo</button></form>");
  envia(200, pagina(F("Configurar"), c));
}

void salvaConfig() {
  String ssid = web.arg("ssid"), wifi = web.arg("wifi"), usuario = web.arg("usuario"), senha = web.arg("senha"), nome = web.arg("nome");
  int pino = web.arg("pino").toInt(), pulso = web.arg("pulso").toInt(); bool alto = web.arg("alto") != "0";
  usuario.trim(); nome.trim(); if (nome.isEmpty()) nome = "Portao";
  bool usuarioOk = usuario.length() >= 3 && usuario.length() <= 32;
  for (size_t i = 0; i < usuario.length(); i++) if (!isalnum((unsigned char)usuario[i])) usuarioOk = false;
  if (ssid.isEmpty() || ssid.length() > 32) return paginaConfig(F("Escolha a rede Wi-Fi."));
  if (wifi.length() > 63) return paginaConfig(F("A senha do Wi-Fi &eacute; longa demais."));
  if (!usuarioOk) return paginaConfig(F("O usu&aacute;rio precisa ter de 3 a 32 letras ou n&uacute;meros, sem espa&ccedil;o."));
  if (senha.length() < 8 || senha.length() > 64) return paginaConfig(F("A senha precisa ter de 8 a 64 caracteres."));
  if (senha != web.arg("senha2")) return paginaConfig(F("As duas senhas n&atilde;o s&atilde;o iguais."));
  if (!pinoValido(pino)) return paginaConfig(F("Pino inv&aacute;lido."));
  if (pulso < 100 || pulso > 3000) return paginaConfig(F("A dura&ccedil;&atilde;o do toque vai de 100 a 3000."));
  String sal = aleatorio(16);
  prefs.begin("hgt", false);
  prefs.putString("ssid", ssid); prefs.putString("wifi", wifi); prefs.putString("usuario", usuario);
  prefs.putString("sal", sal); prefs.putString("hash", sha256hex(sal + ":" + senha)); prefs.putString("nome", nome.substring(0, 24));
  prefs.putInt("pino", pino); prefs.putBool("alto", alto); prefs.putInt("pulso", pulso); prefs.putBool("ok", true);
  prefs.end();
  envia(200, pagina(F("Salvo"), F("<div class=\"cx\"><b>Configura&ccedil;&atilde;o salva.</b><p style=\"margin-top:10px\">A placa vai reiniciar e entrar no Wi-Fi da casa. "
    "Esta rede some. Volte o celular para o seu Wi-Fi e abra <b>http://hgt-portao.local</b> ou o endere&ccedil;o IP que o roteador deu &agrave; placa.</p></div>")));
  delay(1500); ESP.restart();
}

void iniciaModoConfig() {
  modoConfig = true;
  uint8_t mac[6]; WiFi.macAddress(mac); char suf[5]; snprintf(suf, sizeof suf, "%02X%02X", mac[4], mac[5]);
  nomeAp = String("HGT-Portao-") + suf;
  WiFi.mode(WIFI_AP_STA); WiFi.softAP(nomeAp.c_str());
  delay(200); buscaRedes();
  dns.start(53, "*", WiFi.softAPIP());
  web.on("/", HTTP_GET, []() { paginaConfig(); });
  web.on("/buscar", HTTP_GET, []() { buscaRedes(); web.sendHeader("Location", "/"); web.send(303); });
  web.on("/salvar", HTTP_POST, salvaConfig);
  web.onNotFound([]() { web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/"); web.send(302); });
  web.begin();
  Serial.printf("[config] rede %s aberta; abra http://%s\n", nomeAp.c_str(), WiFi.softAPIP().toString().c_str());
}

// ---------- sessão e login
bool autenticado() {
  String ck = web.header("Cookie"); int i = ck.indexOf("hgt=");
  if (i < 0) return false;
  String t = ck.substring(i + 4); int f = t.indexOf(';'); if (f >= 0) t = t.substring(0, f); t.trim();
  if (t.length() != 48) return false;
  uint32_t a = agoraS(); bool ok = false;
  for (auto& s : sessoes) if (s.ate > a && igual(s.token, t)) ok = true;
  return ok;
}

void paginaLogin(const String& erro = "") {
  String c = "<p class=\"sub\">" + esc(cfg.nome) + "</p>";
  if (erro.length()) c += "<div class=\"av\">" + erro + "</div>";
  c += F("<form method=\"post\" action=\"/entrar\"><label>Usu&aacute;rio</label><input name=\"usuario\" maxlength=\"32\" autocapitalize=\"none\" autocomplete=\"username\" required>"
         "<label>Senha</label><input type=\"password\" name=\"senha\" maxlength=\"64\" autocomplete=\"current-password\" required><button type=\"submit\">Entrar</button></form>");
  envia(erro.length() ? 401 : 200, pagina(F("Entrar"), c));
}

void entrar() {
  uint32_t a = agoraS();
  if (a < bloqueadoAte) return paginaLogin(F("Muitas tentativas erradas. Aguarde um minuto e tente de novo."));
  bool ok = igual(web.arg("usuario"), cfg.usuario) & igual(sha256hex(cfg.sal + ":" + web.arg("senha")), cfg.hash);
  if (!ok) {
    if (++falhasLogin >= 5) { bloqueadoAte = a + 60; falhasLogin = 0; }
    delay(400);
    return paginaLogin(F("Usu&aacute;rio ou senha incorretos."));
  }
  falhasLogin = 0;
  int alvo = 0; for (int i = 1; i < MAX_SESSOES; i++) if (sessoes[i].ate < sessoes[alvo].ate) alvo = i;    // reaproveita a mais antiga
  sessoes[alvo].token = aleatorio(24); sessoes[alvo].ate = a + SESSAO_S;
  web.sendHeader("Set-Cookie", "hgt=" + sessoes[alvo].token + "; Path=/; Max-Age=" + String(SESSAO_S) + "; HttpOnly; SameSite=Strict");
  web.sendHeader("Location", "/"); web.send(303);
}

void sair() {
  String ck = web.header("Cookie"); int i = ck.indexOf("hgt=");
  if (i >= 0) { String t = ck.substring(i + 4, i + 52); for (auto& s : sessoes) if (igual(s.token, t)) s.ate = 0; }
  web.sendHeader("Set-Cookie", "hgt=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  web.sendHeader("Location", "/"); web.send(303);
}

// ---------- página principal
void painel() {
  if (!autenticado()) return paginaLogin();
  String c = "<p class=\"sub\">" + esc(cfg.nome) + "</p>";
  c += F("<button id=\"abrir\">Acionar o port&atilde;o</button><p id=\"msg\" class=\"sub\" style=\"margin-top:10px;text-align:center\">&nbsp;</p>"
         "<div class=\"cx\"><div class=\"ln\"><span>Placa</span><b class=\"ok\">ligada</b></div>"
         "<div class=\"ln\"><span>Wi-Fi</span><b id=\"rede\">...</b></div><div class=\"ln\"><span>Sinal</span><b id=\"sinal\">...</b></div>"
         "<div class=\"ln\"><span>Endere&ccedil;o na rede</span><b id=\"ip\">...</b></div><div class=\"ln\"><span>Internet</span><b id=\"net\">...</b></div>"
         "<div class=\"ln\"><span>Ligada h&aacute;</span><b id=\"up\">...</b></div><div class=\"ln\"><span>&Uacute;ltimo acionamento</span><b id=\"ult\">...</b></div></div>"
         "<form method=\"post\" action=\"/sair\"><button class=\"sec\" type=\"submit\">Sair</button></form>"
         "<script>const $=i=>document.getElementById(i);"
         "function t(s){if(s<60)return s+' s';if(s<3600)return Math.floor(s/60)+' min';return Math.floor(s/3600)+' h '+Math.floor(s%3600/60)+' min'}"
         "async function estado(){try{const r=await fetch('/api/estado',{cache:'no-store'});if(r.status==401){location.reload();return}const e=await r.json();"
         "$('rede').textContent=e.rede;$('sinal').textContent=e.sinal+' dBm';$('ip').textContent=e.ip;"
         "$('net').textContent=e.internet?'conectada':'sem resposta';$('net').className=e.internet?'ok':'er';"
         "$('up').textContent=t(e.ligada);$('ult').textContent=e.ultimo<0?'nenhum desde que ligou':'h\\u00e1 '+t(e.ultimo)}catch(x){$('net').textContent='placa sem resposta';$('net').className='er'}}"
         "$('abrir').onclick=async()=>{const b=$('abrir');b.disabled=true;try{const r=await fetch('/api/acionar',{method:'POST',headers:{'X-HGT':'1'}});"
         "$('msg').textContent=r.ok?'Comando enviado.':r.status==429?'Aguarde dois segundos entre um toque e outro.':'N\\u00e3o foi poss\\u00edvel acionar.'}"
         "catch(x){$('msg').textContent='Placa sem resposta.'}setTimeout(()=>{b.disabled=false;$('msg').innerHTML='&nbsp;'},2500);estado()};"
         "estado();setInterval(estado,3000);</script>");
  envia(200, pagina(esc(cfg.nome), c));
}

void apiEstado() {
  if (!autenticado()) { web.send(401, "application/json", "{\"erro\":\"login\"}"); return; }
  String nome = cfg.ssid; nome.replace("\\", "\\\\"); nome.replace("\"", "\\\"");
  String j = "{\"rede\":\"" + nome + "\",\"sinal\":" + String(WiFi.RSSI()) + ",\"ip\":\"" + WiFi.localIP().toString() + "\",\"internet\":" +
    (internetOk ? "true" : "false") + ",\"ligada\":" + String(agoraS()) + ",\"ultimo\":" + String(ultimoToqueS < 0 ? -1 : (int)(agoraS() - ultimoToqueS)) +
    ",\"toques\":" + String(toques) + ",\"versao\":\"" VERSAO "\"}";
  web.sendHeader("Cache-Control", "no-store"); web.send(200, "application/json", j);
}

void apiAcionar() {
  if (!autenticado()) { web.send(401, "application/json", "{\"erro\":\"login\"}"); return; }
  if (web.header("X-HGT") != "1") { web.send(400, "application/json", "{\"erro\":\"pedido\"}"); return; }   // só a própria página manda este cabeçalho
  if (pulsando || (toques && millis() - ultimoToqueMs < INTERVALO_TOQUES_MS)) { web.send(429, "application/json", "{\"erro\":\"aguarde\"}"); return; }
  toque(); web.send(200, "application/json", "{\"ok\":true}");
}

void iniciaModoNormal() {
  releDesligado(); pinMode(cfg.pino, OUTPUT); releDesligado();          // o nível de repouso vai ao pino antes de ele virar saída
  WiFi.mode(WIFI_STA); WiFi.setHostname("hgt-portao"); WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.ssid.c_str(), cfg.senhaWifi.c_str());
  static const char* cabecalhos[] = { "Cookie", "X-HGT" };
  web.collectHeaders(cabecalhos, 2);
  web.on("/", HTTP_GET, painel);
  web.on("/entrar", HTTP_POST, entrar);
  web.on("/sair", HTTP_POST, sair);
  web.on("/api/estado", HTTP_GET, apiEstado);
  web.on("/api/acionar", HTTP_POST, apiAcionar);
  web.onNotFound([]() { web.sendHeader("Location", "/"); web.send(302); });
  web.begin();
  Serial.printf("[wifi] conectando em %s\n", cfg.ssid.c_str());
}

void checaInternet() {
  if (WiFi.status() != WL_CONNECTED) { internetOk = false; return; }
  HTTPClient h; h.setConnectTimeout(3000); h.setTimeout(3000);
  if (h.begin("http://connectivitycheck.gstatic.com/generate_204")) { int c = h.GET(); internetOk = (c == 204 || c == 200); h.end(); } else internetOk = false;
  internetChecada = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_BOOT, INPUT_PULLUP);
  delay(300);
  Serial.println(F("\n>_ Hudson Gomes Tech | Portao com ESP32 " VERSAO " | programa de demonstracao, apenas para laboratorio e estudo"));
  carregaConfig();
  if (cfg.ok) iniciaModoNormal(); else iniciaModoConfig();
  if (!cfg.ok || cfg.pino != PINO_LED) pinMode(PINO_LED, OUTPUT);
}

void loop() {
  static bool estavaConectado = false;
  static uint32_t pisca = 0;
  uint32_t m = millis();
  if (modoConfig) dns.processNextRequest();
  web.handleClient();
  if (pulsando && (int32_t)(m - pulsoAte) >= 0) { releDesligado(); pulsando = false; }

  if (!modoConfig) {
    bool con = WiFi.status() == WL_CONNECTED;
    if (con && !estavaConectado) {
      Serial.printf("[wifi] conectado; abra http://%s ou http://hgt-portao.local\n", WiFi.localIP().toString().c_str());
      MDNS.end(); if (MDNS.begin("hgt-portao")) MDNS.addService("http", "tcp", 80);
      proximaChecagem = m + 2000;
    }
    estavaConectado = con;
    if (con && !pulsando && (int32_t)(m - proximaChecagem) >= 0) { checaInternet(); proximaChecagem = millis() + 30000; }
  }

  // LED: pisca rápido no modo de configuração, devagar procurando o Wi-Fi, apagado quando conectado
  if (!cfg.ok || cfg.pino != PINO_LED) {
    uint32_t periodo = modoConfig ? 200 : (WiFi.status() == WL_CONNECTED ? 0 : 800);
    if (!periodo) digitalWrite(PINO_LED, HIGH);
    else if (m - pisca >= periodo) { pisca = m; digitalWrite(PINO_LED, !digitalRead(PINO_LED)); }
  }

  // BOOT por 5 segundos apaga a configuração
  if (digitalRead(PINO_BOOT) == LOW) { if (!bootDesde) bootDesde = m; else if (m - bootDesde > 5000) { Serial.println(F("[config] apagada pelo botao BOOT")); apagaConfig(); delay(300); ESP.restart(); } }
  else bootDesde = 0;
  delay(2);
}
