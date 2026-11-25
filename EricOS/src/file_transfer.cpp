#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include "file_transfer.h"
#include "storage.h"

namespace {
constexpr char AP_NAME[] = "ERIC-Files";
constexpr char AP_PASSWORD[] = "eosfiles";
constexpr uint16_t HTTP_PORT = 80;
constexpr size_t LINE_CAPACITY = 160;
constexpr size_t NAME_CAPACITY = 96;
constexpr size_t PATH_CAPACITY = 128;
constexpr uint32_t REQUEST_TIMEOUT_MS = 5000;



constexpr uint32_t UPLOAD_TIMEOUT_MS = 30000UL;













constexpr size_t WRITE_CHUNK = 128;
constexpr size_t READ_CHUNK = 128;
constexpr size_t LINE_OUT = 768;
constexpr uint16_t LIST_MAX_ENTRIES = 200;

WiFiServer server(HTTP_PORT);
WiFiClient client;
EricTransfer::State transferState = EricTransfer::State::Off;
char transferMessage[48] = "OFF";
char transferIp[20] = "192.168.4.1";
uint32_t bytesReceived = 0;
uint32_t contentRemaining = 0;
uint32_t uploadStartedAt = 0;
File uploadFile;
char uploadPath[NAME_CAPACITY + 20] = {};
uint8_t copyBuffer[READ_CHUNK] = {};




enum class RequestState : uint8_t { Idle, RequestLine, Headers, Complete };
RequestState requestState = RequestState::Idle;
char requestLine[LINE_CAPACITY] = "";
uint32_t requestContentLength = 0;
uint32_t requestStartedAt = 0;
char lineBuffer[LINE_CAPACITY] = "";
size_t lineUsed = 0;




enum class Out : uint8_t { None, Page, ListHead, ListBody, ListFoot, FileHead, FileBody, Note, Finish };
Out out = Out::None;
uint32_t bytesSent = 0;





const char *outCursor = nullptr;


char outNote[288] = "";



char uploadName[NAME_CAPACITY] = "";
char lineOut[LINE_OUT] = "";






uint8_t pageCursorStep = 0;
uint8_t listTitleStep = 0;
uint8_t listFootStep = 0;




char fileHeader[288] = "";



File listDir;
char listPath[PATH_CAPACITY] = "/";
uint16_t listedCount = 0;
bool listTruncated = false;




File downloadFile;
char downloadPath[PATH_CAPACITY] = "/";
char downloadName[NAME_CAPACITY] = "";
uint32_t downloadRemaining = 0;
size_t pendingOff = 0;
size_t pendingLen = 0;









EricTransfer::Direction transferDirection = EricTransfer::Direction::Idle;
uint32_t fileBytesTotal = 0;
uint32_t fileBytesDone = 0;















bool apUp = false;
uint32_t apIdleSince = 0;



constexpr uint32_t AP_IDLE_SHUTDOWN_MS = 180000;



void resetProgress() {
  transferDirection = EricTransfer::Direction::Idle;
  fileBytesTotal = 0;
  fileBytesDone = 0;
}

void setMessage(const char *message) {
  strncpy(transferMessage, message, sizeof(transferMessage) - 1);
  transferMessage[sizeof(transferMessage) - 1] = '\0';
}

void resetRequest() {
  requestState = RequestState::Idle;
  requestLine[0] = '\0';
  requestContentLength = 0;
  lineUsed = 0;
}

void closeStreams() {
  if (uploadFile) uploadFile.close();
  if (listDir) listDir.close();
  if (downloadFile) downloadFile.close();
  out = Out::None;
  outCursor = nullptr;
  contentRemaining = 0;
  downloadRemaining = 0;
  pendingOff = 0;
  pendingLen = 0;
  
  
  
  
  if (transferState != EricTransfer::State::Complete) resetProgress();
}

void finishClient() {
  closeStreams();
  client.stop();
}



size_t writeOut(const char *data, size_t size) {
  if (data == nullptr || size == 0) return 0;
  const int room = client.availableForWrite();
  if (room <= 0) return 0;
  size_t want = static_cast<size_t>(room);
  if (want > size) want = size;
  if (want > WRITE_CHUNK) want = WRITE_CHUNK;
  const size_t sent = client.write(reinterpret_cast<const uint8_t *>(data), want);
  bytesSent += sent;
  return sent;
}

void emit(const char *text) { outCursor = text; }

bool cursorDone() { return outCursor == nullptr || outCursor[0] == '\0'; }




bool flushCursor() {
  while (!cursorDone()) {
    const size_t sent = writeOut(outCursor, strlen(outCursor));
    if (sent == 0) return false;
    outCursor += sent;
  }
  outCursor = nullptr;
  return true;
}

void startPage() {
  out = Out::Page;
  outCursor = nullptr;
  
  
  
  pageCursorStep = 0;
  bytesSent = 0;
}

void startNote(const char *heading, const char *body) {
  snprintf(outNote, sizeof(outNote),
           "<h1>%s</h1><p>%s</p><p><a href='/'>Back to E.R.I.C. Files</a></p>", heading, body);
  out = Out::Note;
  outCursor = nullptr;
  pageCursorStep = 0;
}

void startListing(const char *path) {
  strncpy(listPath, path, sizeof(listPath) - 1);
  listPath[sizeof(listPath) - 1] = '\0';
  if (listDir) listDir.close();
  listDir = SD.open(listPath);
  listedCount = 0;
  listTruncated = false;
  out = Out::ListHead;
  outCursor = nullptr;
  listTitleStep = 0;
  listFootStep = 0;
  bytesSent = 0;
}

void startDownload(const char *path) {
  strncpy(downloadPath, path, sizeof(downloadPath) - 1);
  downloadPath[sizeof(downloadPath) - 1] = '\0';
  const char *base = strrchr(downloadPath, '/');
  base = base != nullptr ? base + 1 : downloadPath;
  strncpy(downloadName, base, sizeof(downloadName) - 1);
  downloadName[sizeof(downloadName) - 1] = '\0';
  if (downloadFile) downloadFile.close();
  downloadFile = SD.open(downloadPath, FILE_READ);
  downloadRemaining = downloadFile ? static_cast<uint32_t>(downloadFile.size()) : 0;
  pendingOff = 0;
  pendingLen = 0;
  transferDirection = EricTransfer::Direction::Outgoing;
  fileBytesTotal = downloadRemaining;
  fileBytesDone = 0;
  out = Out::FileHead;
  outCursor = nullptr;
  bytesSent = 0;
}






size_t encodeInto(char *dst, size_t capacity, const char *value) {
  size_t used = 0;
  for (const char *p = value; *p != '\0'; ++p) {
    const unsigned char c = static_cast<unsigned char>(*p);
    char encoded[4];
    size_t length;
    if (c == ' ') { encoded[0] = '+'; length = 1; }
    else if (c == '%' || c == '#' || c == '?' || c == '&' || c == '+' || c == '=' ||
             c == '\'' || c < 0x21 || c > 0x7e) {
      snprintf(encoded, sizeof(encoded), "%%%02X", c);
      length = 3;
    } else {
      encoded[0] = static_cast<char>(c);
      length = 1;
    }
    if (used + length >= capacity) return 0;
    memcpy(dst + used, encoded, length);
    used += length;
  }
  dst[used] = '\0';
  return used == 0 ? 0 : used;
}



size_t escapeInto(char *dst, size_t capacity, const char *value) {
  size_t used = 0;
  for (const char *p = value; *p != '\0'; ++p) {
    const char *piece = nullptr;
    char literal[2] = {*p, '\0'};
    if (*p == '&') piece = "&amp;";
    else if (*p == '\'') piece = "&#39;";
    else if (static_cast<unsigned char>(*p) < 0x20) { literal[0] = '.'; piece = literal; }
    else piece = literal;
    const size_t length = strlen(piece);
    if (used + length >= capacity) break;
    memcpy(dst + used, piece, length);
    used += length;
  }
  dst[used] = '\0';
  return used;
}

const char *contentTypeFor(const char *name) {
  const char *dot = strrchr(name, '.');
  if (dot == nullptr) return "application/octet-stream";
  if (strcasecmp(dot, ".mp3") == 0) return "audio/mpeg";
  if (strcasecmp(dot, ".wav") == 0) return "audio/wav";
  if (strcasecmp(dot, ".ogg") == 0) return "audio/ogg";
  if (strcasecmp(dot, ".m4a") == 0) return "audio/mp4";
  if (strcasecmp(dot, ".txt") == 0) return "text/plain";
  if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0) return "text/html";
  if (strcasecmp(dot, ".json") == 0) return "application/json";
  if (strcasecmp(dot, ".csv") == 0) return "text/csv";
  if (strcasecmp(dot, ".png") == 0) return "image/png";
  if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0) return "image/jpeg";
  if (strcasecmp(dot, ".bmp") == 0) return "image/bmp";
  if (strcasecmp(dot, ".gif") == 0) return "image/gif";
  return "application/octet-stream";
}

const char HEAD[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n";
const char STYLE[] = "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
                     "<title>E.R.I.C. Files</title><style>body{background:#000;color:#39ff14;"
                     "font-family:monospace;max-width:40rem;margin:1.5rem auto;padding:0 1rem}"
                     "a{color:#39ff14}button{background:#39ff14;color:#000;border:0;padding:.5rem 1rem;"
                     "font:inherit;margin:.5rem 0}small{color:#8ccc8c}li{list-style:none;padding:.15rem 0}"
                     "h1{font-size:1.1rem}"
                     "#track{height:16px;background:#002200;border:1px solid #39ff14;margin:.6rem 0;display:none}"
                     "#fill{height:100%;width:0;background:#39ff14}"
                     "#note{color:#8ccc8c;margin:.3rem 0}#done{color:#39ff14;font-weight:bold}</style>";
const char PAGE_A[] = "<h1>E.R.I.C. FILE TRANSFER</h1><p>Over Wi-Fi, straight to the microSD card.</p>"
                      "<p><a href='/list?path=%2F'>Browse the card</a> &middot; <a href='/list?path=%2FERIC%2FINBOX'>Inbox</a></p>";



const char PAGE_B[] = "<form><label>Send a file to <b>/ERIC/INBOX/</b><br><input id=f type=file required></label><br>"
                      "<button type=button onclick=send()>Upload to E.R.I.C.</button></form>"
                      "<div id=track><div id=fill></div></div><p id=note></p><p id=done></p>"
                      "<script>function send(){var f=document.getElementById('f').files[0];if(!f)return;"
                      "var t=document.getElementById('track'),fl=document.getElementById('fill'),"
                      "n=document.getElementById('note'),d=document.getElementById('done');"
                      "t.style.display='block';fl.style.width='0%';n.textContent='starting...';d.textContent='';"
                      "var x=new XMLHttpRequest();"
                      "x.open('POST','/upload?name='+encodeURIComponent(f.name));"
                      "x.upload.onprogress=function(e){if(!e.lengthComputable)return;"
                      "var p=Math.floor(e.loaded/e.total*100);fl.style.width=p+'%';"
                      "n.textContent=p+'%  ('+e.loaded+' of '+e.total+' bytes)';};"
                      "x.onload=function(){if(x.status===200){t.style.display='none';n.textContent='';"
                      "document.body.innerHTML=x.responseText;}"
                      "else{n.textContent='The device refused the file (status '+x.status+').';}};"
                      "x.onerror=function(){n.textContent='The connection dropped. Nothing was saved.';};"
                      "x.send(f);}</script>";
const char PAGE_C[] = "<p><small>One file at a time. Transfers need patience, not attention. "
                      "Uploads land in /ERIC/INBOX/.</small></p>";




const char SAVED_HEAD[] = "<h1 id=done>UPLOAD COMPLETE</h1>";
const char SAVED_LINK[] = "<p><a href='/'>Back to E.R.I.C. Files</a> &middot; "
                          "<a href='/list?path=%2FERIC%2FINBOX'>Open the Inbox</a></p>";



void formatSize(char *out, size_t capacity, uint32_t bytes) {
  if (bytes < 1024UL) {
    snprintf(out, capacity, "%lu B", static_cast<unsigned long>(bytes));
  } else if (bytes < 1048576UL) {
    snprintf(out, capacity, "%lu.%01lu KB", static_cast<unsigned long>(bytes / 1024UL),
             static_cast<unsigned long>((bytes % 1024UL) * 10UL / 1024UL));
  } else {
    snprintf(out, capacity, "%lu.%02lu MB", static_cast<unsigned long>(bytes / 1048576UL),
             static_cast<unsigned long>((bytes % 1048576UL) * 100UL / 1048576UL));
  }
}




void startSaved(const char *name, uint32_t size) {
  char pretty[20] = "";
  formatSize(pretty, sizeof(pretty), size);
  snprintf(outNote, sizeof(outNote),
           "%s<p><b>%s</b><br>%s &middot; saved to /ERIC/INBOX/</p>%s",
           SAVED_HEAD, name, pretty, SAVED_LINK);
  out = Out::Note;
  outCursor = nullptr;
  pageCursorStep = 0;
}



bool buildEntryLine(const char *fullPath, const char *base, bool isDirectory, uint32_t size) {
  char href[PATH_CAPACITY * 3 + 24] = {};
  char label[NAME_CAPACITY + 8] = {};
  if (encodeInto(href, sizeof(href), fullPath) == 0) return false;
  if (escapeInto(label, sizeof(label), base) == 0) return false;
  char detail[48] = {};
  if (!isDirectory) {
    if (size >= 1048576UL) {
      snprintf(detail, sizeof(detail), " &nbsp; <small>%lu.%01lu MB</small>",
               static_cast<unsigned long>(size / 1048576UL),
               static_cast<unsigned long>((size / 104857UL) % 10));
    } else if (size >= 1024UL) {
      snprintf(detail, sizeof(detail), " &nbsp; <small>%lu KB</small>", static_cast<unsigned long>(size / 1024UL));
    } else {
      snprintf(detail, sizeof(detail), " &nbsp; <small>%lu B</small>", static_cast<unsigned long>(size));
    }
  } else {
    snprintf(detail, sizeof(detail), " &nbsp; <small>folder</small>");
  }
  const int written = isDirectory
      ? snprintf(lineOut, sizeof(lineOut), "<li><a href='/list?path=%s'>%s/</a>%s</li>", href, label, detail)
      : snprintf(lineOut, sizeof(lineOut), "<li><a href='/get?path=%s'>%s</a>%s</li>", href, label, detail);
  return written > 0 && static_cast<size_t>(written) < sizeof(lineOut);
}



bool stepOut() {
  switch (out) {
    case Out::Page: {
      switch (pageCursorStep) {
        case 0: emit(HEAD); break;
        case 1: emit(STYLE); break;
        case 2: emit(PAGE_A); break;
        case 3: emit(PAGE_B); break;
        case 4: emit(PAGE_C); break;
        default: out = Out::Finish; return true;
      }
      if (!flushCursor()) return false;
      ++pageCursorStep;
      return true;
    }

    case Out::ListHead: {
      if (listDir && !listDir.isDirectory()) {
        
        
        startDownload(listPath);
        return true;
      }
      if (!listDir) { startNote("NO SUCH FOLDER", "That folder is not on the card, or the card changed."); return true; }
      if (listTitleStep == 0) { emit(HEAD); if (!flushCursor()) return false; listTitleStep = 1; }
      if (listTitleStep == 1) { emit(STYLE); if (!flushCursor()) return false; listTitleStep = 2; }
      if (listTitleStep == 2) {
        
        
        const int written = snprintf(lineOut, sizeof(lineOut),
            "<h1>%s</h1><p><a href='/list?path=%%2F'>Card root</a> &middot; <a href='/'>Home</a></p><ul>",
            listPath);
        if (written <= 0 || static_cast<size_t>(written) >= sizeof(lineOut)) {
          startNote("NO SUCH FOLDER", "That folder is not on the card.");
          return true;
        }
        listTitleStep = 3;
        emit(lineOut);
      }
      if (listTitleStep == 3) {
        if (!flushCursor()) return false;
        out = Out::ListBody;
      }
      return true;
    }

    case Out::ListBody: {
      
      
      
      if (!cursorDone()) return flushCursor();
      if (client.availableForWrite() <= 0) return false;
      if (!listDir) { out = Out::ListFoot; return true; }
      if (listedCount >= LIST_MAX_ENTRIES) { listTruncated = true; out = Out::ListFoot; return true; }
      File entry = listDir.openNextFile();
      if (!entry) { out = Out::ListFoot; return true; }
      const bool isDirectory = entry.isDirectory();
      const char *name = entry.name();
      const char *base = strrchr(name, '/');
      base = base != nullptr ? base + 1 : name;
      char full[PATH_CAPACITY];
      
      
      
      const int written = strcmp(listPath, "/") == 0
          ? snprintf(full, sizeof(full), "/%s", base)
          : snprintf(full, sizeof(full), "%s/%s", listPath, base);
      if (written <= 0 || static_cast<size_t>(written) >= sizeof(full)) { listTruncated = true; out = Out::ListFoot; return true; }
      if (!buildEntryLine(full, base, isDirectory, entry.size())) { listTruncated = true; out = Out::ListFoot; return true; }
      emit(lineOut);
      ++listedCount;
      return true;
    }

    case Out::ListFoot: {
      if (listFootStep == 0) { emit("</ul>"); if (!flushCursor()) return false; listFootStep = 1; }
      if (listFootStep == 1) {
        if (!listTruncated) { out = Out::Finish; return true; }
        const int written = snprintf(lineOut, sizeof(lineOut),
            "<p><small>Listing stopped after %u entries. Open a folder to see the rest.</small></p>", listedCount);
        if (written > 0) emit(lineOut);
        if (!flushCursor()) return false;
        listFootStep = 2;
      }
      out = Out::Finish;
      return true;
    }

    case Out::FileHead: {
      if (!downloadFile) { startNote("NOT FOUND", "That file is not on the card."); return true; }
      const int written = snprintf(fileHeader, sizeof(fileHeader),
          "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lu\r\n"
          "Content-Disposition: attachment; filename=\"%s\"\r\nConnection: close\r\n\r\n",
          contentTypeFor(downloadName), static_cast<unsigned long>(downloadRemaining), downloadName);
      if (written <= 0 || static_cast<size_t>(written) >= sizeof(fileHeader)) {
        startNote("CANNOT SEND", "That file name cannot be used in a download header.");
        return true;
      }
      emit(fileHeader);
      if (!flushCursor()) return false;
      out = Out::FileBody;
      return true;
    }

    case Out::FileBody: {
      
      
      if (pendingOff < pendingLen) {
        const size_t accepted = writeOut(reinterpret_cast<const char *>(copyBuffer) + pendingOff, pendingLen - pendingOff);
        pendingOff += accepted;
        fileBytesDone += accepted;
        if (pendingOff < pendingLen) return false;
        pendingOff = 0;
        pendingLen = 0;
      }
      if (downloadRemaining == 0) {
        if (downloadFile) downloadFile.close();
        out = Out::Finish;
        return true;
      }
      if (client.availableForWrite() <= 0) return false;
      size_t want = READ_CHUNK;
      if (want > downloadRemaining) want = downloadRemaining;
      const size_t read = downloadFile.read(copyBuffer, want);
      if (read == 0) {
        
        
        if (downloadFile) downloadFile.close();
        transferState = EricTransfer::State::Error;
        setMessage("SD READ FAILED");
        out = Out::Finish;
        return true;
      }
      downloadRemaining -= read;
      pendingOff = 0;
      pendingLen = read;
      return true;
    }

    case Out::Note: {
      if (pageCursorStep == 0) { emit(HEAD); if (!flushCursor()) return false; ++pageCursorStep; }
      if (pageCursorStep == 1) { emit(STYLE); if (!flushCursor()) return false; ++pageCursorStep; }
      if (pageCursorStep == 2) { emit(outNote); if (!flushCursor()) return false; ++pageCursorStep; }
      out = Out::Finish;
      return true;
    }

    case Out::Finish:
      client.stop();
      out = Out::None;
      outCursor = nullptr;
      if (transferState == EricTransfer::State::Complete) {
        transferState = EricTransfer::State::Ready;
        setMessage("READY FOR PHONE");
      }
      return true;

    case Out::None:
    default:
      return true;
  }
}



void pumpOut() {
  uint8_t quiet = 0;
  while (out != Out::None && quiet < 2) {
    const size_t before = bytesSent;
    stepOut();
    if (out == Out::None) return;
    
    
    
    
    if (bytesSent == before) ++quiet;
  }
}




void urlDecode(char *value) {
  size_t read = 0;
  size_t wrote = 0;
  while (value[read] != '\0' && value[read] != ' ' && value[read] != '&' &&
         value[read] != '\n' && value[read] != '\r') {
    if (value[read] == '+') {
      value[wrote++] = ' ';
      ++read;
    } else if (value[read] == '%' && isxdigit(static_cast<unsigned char>(value[read + 1])) &&
               isxdigit(static_cast<unsigned char>(value[read + 2]))) {
      const char hex[3] = {value[read + 1], value[read + 2], '\0'};
      value[wrote++] = static_cast<char>(strtoul(hex, nullptr, 16));
      read += 3;
    } else {
      value[wrote++] = value[read++];
    }
  }
  value[wrote] = '\0';
}

bool queryValue(const char *request, const char *key, char *out, size_t capacity) {
  const char *found = strstr(request, key);
  if (found == nullptr || capacity < 2) return false;
  found += strlen(key);
  size_t used = 0;
  while (*found != '\0' && *found != ' ' && used + 1 < capacity) out[used++] = *found++;
  out[used] = '\0';
  urlDecode(out);
  return out[0] != '\0';
}





bool safeCardPath(const char *input, char *out, size_t capacity) {
  if (input == nullptr) return false;
  const size_t length = strlen(input);
  if (length == 1 && input[0] == '/') {
    if (capacity < 2) return false;
    strcpy(out, "/");
    return true;
  }
  if (input[0] != '/') return false;
  if (length < 2 || length >= capacity) return false;
  for (size_t i = 0; i < length; ++i) {
    const unsigned char c = static_cast<unsigned char>(input[i]);
    if (c < 0x20 || c == 0x7f) return false;   
    if (c == '\\' || c == '*' || c == '?') return false;
  }
  if (input[length - 1] == '/') return false;   
  
  
  
  const char *segment = input + 1;
  while (*segment != '\0') {
    const char *slash = strchr(segment, '/');
    const size_t segmentLength = slash != nullptr ? static_cast<size_t>(slash - segment) : strlen(segment);
    if (segmentLength == 0) return false;                       
    if (segmentLength == 1 && segment[0] == '.') return false;  
    if (segmentLength == 2 && segment[0] == '.' && segment[1] == '.') return false;
    if (slash == nullptr) break;
    segment = slash + 1;
  }
  strncpy(out, input, capacity - 1);
  out[capacity - 1] = '\0';
  return true;
}





bool safeFilename(const char *input, char *output, size_t outputSize) {
  if (input == nullptr || outputSize < 2) return false;
  const char *base = strrchr(input, '/');
  base = base ? base + 1 : input;
  size_t used = 0;
  for (size_t i = 0; base[i] != '\0' && used + 1 < outputSize; ++i) {
    const unsigned char c = static_cast<unsigned char>(base[i]);
    const bool keep = isalnum(c) || c == '.' || c == '_' || c == '-';
    output[used++] = keep ? base[i] : '_';
  }
  output[used] = '\0';
  return used > 0 && strcmp(output, ".") != 0 && strcmp(output, "..") != 0;
}





bool readLine(char *out, size_t capacity) {
  while (client.connected() && client.available()) {
    const char c = static_cast<char>(client.read());
    if (c == '\n') {
      lineBuffer[lineUsed] = '\0';
      strncpy(out, lineBuffer, capacity - 1);
      out[capacity - 1] = '\0';
      lineUsed = 0;
      return true;
    }
    if (c != '\r' && lineUsed + 1 < sizeof(lineBuffer)) lineBuffer[lineUsed++] = c;
  }
  return false;
}

bool beginUpload(const char *request, uint32_t length) {
  char name[NAME_CAPACITY + 1] = {};
  if (!queryValue(request, "name=", name, sizeof(name)) || length == 0) return false;
  char fileName[NAME_CAPACITY] = {};
  if (!safeFilename(name, fileName, sizeof(fileName))) return false;
  strncpy(uploadName, fileName, sizeof(uploadName) - 1);
  uploadName[sizeof(uploadName) - 1] = '\0';
  if (!SD.exists("/ERIC") && !SD.mkdir("/ERIC")) return false;
  if (!SD.exists("/ERIC/INBOX") && !SD.mkdir("/ERIC/INBOX")) return false;
  snprintf(uploadPath, sizeof(uploadPath), "/ERIC/INBOX/%s", fileName);
  uploadFile = SD.open(uploadPath, FILE_WRITE);
  if (!uploadFile) return false;
  uploadFile.seek(0);
  uploadFile.truncate(0);
  contentRemaining = length;
  bytesReceived = 0;
  transferDirection = EricTransfer::Direction::Incoming;
  fileBytesTotal = length;
  fileBytesDone = 0;
  uploadStartedAt = millis();
  transferState = EricTransfer::State::Receiving;
  setMessage("RECEIVING...");
  return true;
}
}

namespace EricTransfer {
bool start() {
  
  
  
  if (!EricStorage::isReady()) { transferState = State::Error; setMessage("INSERT FAT32 SD CARD"); return false; }
  if (!apUp) {
    
    
    WiFi.mode(WIFI_AP_STA);
    if (!WiFi.beginAP(AP_NAME, AP_PASSWORD)) { transferState = State::Error; setMessage("HOTSPOT DID NOT START"); return false; }
    apUp = true;
    const String ip = WiFi.softAPIP().toString();
    strncpy(transferIp, ip.c_str(), sizeof(transferIp) - 1);
    transferIp[sizeof(transferIp) - 1] = '\0';
  }
  
  
  finishClient();
  server.begin();
  resetRequest();
  resetProgress();
  bytesSent = 0;
  bytesReceived = 0;
  apIdleSince = millis();
  transferState = State::Ready;
  setMessage("READY FOR PHONE");
  return true;
}




void shutdownHotspot() {
  if (!apUp) return;
  finishClient();
  server.stop();
  
  
  
  WiFi.softAPdisconnect(false);
  apUp = false;
  transferState = State::Off;
  setMessage("HOTSPOT CLOSED");
}



void maintenance() {
  if (!apUp) return;
  if (WiFi.softAPgetStationNum() > 0) { apIdleSince = millis(); return; }
  if (millis() - apIdleSince < AP_IDLE_SHUTDOWN_MS) return;
  shutdownHotspot();
}

void stop() {
  
  
  
  
  finishClient();
  server.stop();
  transferState = State::Off;
  setMessage("TRANSFER STOPPED");
}

void pump() {
  if (transferState == State::Off) return;

  
  if (transferState == State::Receiving) {
    const int ready = client.available();
    if (ready > 0) {
      const size_t count = static_cast<size_t>(ready) < READ_CHUNK ? static_cast<size_t>(ready) : READ_CHUNK;
      const size_t received = client.read(copyBuffer, count);
      const size_t written = uploadFile.write(copyBuffer, received);
      bytesReceived += written;
      
      
      
      fileBytesDone = bytesReceived;
      contentRemaining = written > contentRemaining ? 0 : contentRemaining - written;
      if (written != received) { finishClient(); SD.remove(uploadPath); transferState = State::Error; setMessage("SD WRITE FAILED"); return; }
      
      
      
      uploadStartedAt = millis();
    }
    if (contentRemaining == 0) {
      uploadFile.close();
      transferState = State::Complete;
      setMessage("FILE SAVED TO INBOX");
      
      
      fileBytesDone = fileBytesTotal;
      startSaved(uploadName, bytesReceived);
      pumpOut();
    } else if (millis() - uploadStartedAt > UPLOAD_TIMEOUT_MS) {
      finishClient(); SD.remove(uploadPath); transferState = State::Error; setMessage("UPLOAD TIMED OUT");
    }
    return;
  }

  
  if (out != Out::None) {
    if (!client.connected()) { closeStreams(); transferState = State::Ready; setMessage("READY FOR PHONE"); return; }
    pumpOut();
    return;
  }

  if (!client || !client.connected()) {
    if (uploadFile) uploadFile.close();
    resetRequest();
    client = server.accept();
    if (!client) return;
    
    client.setNoDelay(true);
  }

  if (requestState == RequestState::Idle) {
    if (!client.available()) return;
    requestState = RequestState::RequestLine;
    requestStartedAt = millis();
  }
  if (millis() - requestStartedAt > REQUEST_TIMEOUT_MS) {
    client.stop();
    resetRequest();
    return;
  }
  char line[LINE_CAPACITY] = {};
  if (requestState == RequestState::RequestLine) {
    if (!readLine(line, sizeof(line))) return;
    strncpy(requestLine, line, sizeof(requestLine) - 1);
    requestLine[sizeof(requestLine) - 1] = '\0';
    requestState = RequestState::Headers;
  }
  while (requestState == RequestState::Headers && readLine(line, sizeof(line))) {
    if (line[0] == '\0') { requestState = RequestState::Complete; break; }
    if (strncasecmp(line, "Content-Length:", 15) == 0) requestContentLength = strtoul(line + 15, nullptr, 10);
  }
  if (requestState != RequestState::Complete) return;

  char requested[PATH_CAPACITY] = {};
  char checked[PATH_CAPACITY] = {};
  
  
  
  
  resetProgress();
  if (strncmp(requestLine, "POST /upload?", 13) == 0) {
    if (!beginUpload(requestLine, requestContentLength)) {
      
      setMessage("UPLOAD REJECTED - SIMPLER FILENAME");
      startNote("UPLOAD NOT ACCEPTED", "The filename was rejected or the card refused the write. "
                                       "Try a plain name with letters, digits, dot, dash or underscore.");
    }
  } else if (strncmp(requestLine, "GET /list?", 10) == 0) {
    queryValue(requestLine, "path=", requested, sizeof(requested));
    if (!safeCardPath(requested, checked, sizeof(checked))) {
      startListing("/");
    } else {
      startListing(checked);
    }
  } else if (strncmp(requestLine, "GET /get?", 9) == 0) {
    
    
    
    const bool valid = queryValue(requestLine, "path=", requested, sizeof(requested)) &&
                       safeCardPath(requested, checked, sizeof(checked)) &&
                       SD.exists(checked) && !SD.open(checked).isDirectory();
    if (valid) {
      startDownload(checked);
    } else {
      startNote("FILE NOT FOUND", "That file is not on the card, or the card changed. "
                                  "Go back and pick something from the listing.");
    }
  } else {
    startPage();
  }
  resetRequest();
  pumpOut();
}

State state() { return transferState; }
Direction direction() { return transferDirection; }
bool hotspotUp() { return apUp; }
const char *message() { return transferMessage; }
const char *ipAddress() { return transferIp; }
uint32_t receivedBytes() { return bytesReceived; }
uint32_t sentBytes() { return bytesSent; }
uint32_t progressDone() { return fileBytesDone; }
uint32_t progressTotal() { return fileBytesTotal; }
}
