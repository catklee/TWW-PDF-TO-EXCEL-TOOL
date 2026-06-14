#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace {

struct CartItem {
  string product;
  string option;
  string quantity;
  string price;
};

//removing leading & tailing whitespace
string trim(const string& value) {
  const auto start = value.find_first_not_of(" \t\r\n");
  if (start ==  string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

//removing in-between whitespace
string collapse_spaces(const  string& value) {
  string  output; output.reserve(value.size());
  bool in_space = false;
  for (unsigned char ch : value) {
    //skip control characters
    if (ch < 32) {
      continue;
    }
    //current character is empty space
    if (ch == ' ' || ch == '\t') {
      //and if we didn't add an empty space already
      if (!in_space && ! output.empty()) {
        output.push_back(' ');
        //add one
        in_space = true;
      }
      //if we did already, move to next letter
      continue;
    }
    //resets in_space; also add current letter bc it actually matters
    output.push_back(static_cast<char>(ch));
    in_space = false;
  }
  return trim(output);
}
//unicode & icon stripping
bool is_private_use_codepoint(unsigned int codepoint) {
  return (codepoint >= 0xE000 && codepoint <= 0xF8FF) ||
         (codepoint >= 0xF0000 && codepoint <= 0xFFFFD) ||
         (codepoint >= 0x100000 && codepoint <= 0x10FFFD);
}

unsigned int next_codepoint(const  string& value, size_t& index) {
  const unsigned char ch = static_cast<unsigned char>(value[index]);
  if ((ch & 0xF8) == 0xF0) {
    const unsigned int cp =
        ((value[index] & 0x07U) << 18) | ((static_cast<unsigned char>(value[index + 1]) & 0x3FU) << 12) |
        ((static_cast<unsigned char>(value[index + 2]) & 0x3FU) << 6) |
        (static_cast<unsigned char>(value[index + 3]) & 0x3FU);
    index += 4;
    return cp;
  }
  if ((ch & 0xF0) == 0xE0) {
    const unsigned int cp = ((value[index] & 0x0FU) << 12) |
                            ((static_cast<unsigned char>(value[index + 1]) & 0x3FU) << 6) |
                            (static_cast<unsigned char>(value[index + 2]) & 0x3FU);
    index += 3;
    return cp;
  }
  if ((ch & 0xE0) == 0xC0) {
    const unsigned int cp =
        ((value[index] & 0x1FU) << 6) | (static_cast<unsigned char>(value[index + 1]) & 0x3FU);
    index += 2;
    return cp;
  }
  const unsigned int cp = value[index];
  index += 1;
  return cp;
}

void append_codepoint( string&  output, unsigned int codepoint) {
  if (codepoint <= 0x7F) {
     output.push_back(static_cast<char>(codepoint));
    return;
  }
  if (codepoint <= 0x7FF) {
     output.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
     output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    return;
  }
  if (codepoint <= 0xFFFF) {
     output.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
     output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
     output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    return;
  }
   output.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
   output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
   output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
   output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
}

 string strip_icons(const  string& value) {
   string  output;
   output.reserve(value.size());
  for (size_t i = 0; i < value.size();) {
    const unsigned int cp = next_codepoint(value, i);
    if (cp < 0x20) {
      continue;
    }
    if (is_private_use_codepoint(cp)) {
      continue;
    }
    append_codepoint( output, cp);
  }
  return output;
}

string normalize(const string& value) {
  return collapse_spaces(strip_icons(value));
}

size_t leading_spaces(const  string& value) {
  return value.find_first_not_of(' ') ==  string::npos
             ? value.size()
             : value.find_first_not_of(' ');
}

bool contains(const  string& haystack, const  string& needle) {
  return haystack.find(needle) !=  string::npos;
}

bool ends_with(const  string& value, const  string& suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_date(const string& line) {
  static const regex date_pattern(R"(\b\d{1,2}/\d{1,2}/\b)");
  return regex_search(line, date_pattern);
}

bool is_noise(const  string& line) {
  static const  array<const char*, 15> kNoise = {
      "https://", "[기본배송]", "양말전문쇼핑몰", "FAKESOCKS", "LOGOUT",
      "LeeKim", "국내배송", "해외배송", "일반상품", "혜택정보", "가용적립", "쿠폰",
      "장바구니", "COPYRIGHT", "     3,000원"};

  if (is_date(line)) {
    return true;
  }
  if (line.empty()) {
    return true;
  }
  if (line == "주문하기" || line == "관심상품등록" || line == "옵션변경" ||
      line == "변경" || line == "삭제" || line == "차등" || line == "0원" ||
      line == "-") {
    return true;
  }
  if (contains(line, "이미지") && contains(line, "상품정보")) {
    return true;
  }
  for (const char* part : kNoise) {
    if (contains(line, part)) {
      return true;
    }
  }
  return false;
}

bool is_meta_line(const  string& line) {
  if (is_noise(line)) {
    return true;
  }
  if (contains(line, "관심상품") || contains(line, "옵션변경") || line == "삭제") {
    return true;
  }
  if (line == "차등" || ends_with(line, " 차등") || ends_with(line, " 변경")) {
    return true;
  }
  static const  regex pack_re(R"((\d+켤레|\d+장\b|vat별도|^\d+종|색상선택|택\d))");
  if ( regex_search(line, pack_re)) {
    return true;
  }
  if (contains(line, "기본배송")) {
    return true;
  }
  static const  regex price_re(R"(\d+[,]?\d*원)");
  if ( regex_search(line, price_re)) {
    if ( regex_match(line,  regex(R"([\d,.\s원+-]+)"))) {
      return true;
    }
    if (line.size() < 35 && !contains(line, "켤레")) {
      return true;
    }
  }
  return false;
}

bool is_stop_backtrack_line(const  string& raw) {
  const  string line = normalize(raw);
  if (line.empty()) {
    return false;
  }
  if (contains(line, "vat별도")) {
    return true;
  }
  if ( regex_match(line,  regex(R"(\d+장.*)"))) {
    return true;
  }
  return false;
}

 string extract_option(const  string& raw) {
  static const  regex option_re(R"(\[옵션:\s*([^\]]+)\])");
   smatch match;
  if ( regex_search(raw, match, option_re)) {
    return trim(match[1].str());
  }
  return "";
}

string extract_price(const string& row) {
  static const regex price_re(R"((\d[\d,]*)\s*원)");
  smatch match;
  if (regex_search(row, match, price_re)) {
    const string p = match[1].str() + "원";
    if (p == "0원") return "";
    return p;
  }
  return "";
}

bool extract_qty_only(const  string& raw,  string& qty) {
  static const  regex qty_re(R"(^\s{20,}(\d{1,4})(?:\s|$))");
   smatch match;
  if ( regex_search(raw, match, qty_re)) {
    const size_t digits_start = static_cast<size_t>(match.position(1));
    const  string before = raw.substr(0, digits_start);
    if (before.find_first_not_of(' ') !=  string::npos) {
      return false;
    }
    qty = match[1].str();
    return true;
  }
  return false;
}

bool extract_qty_tail(const  string& line,  string& name,  string& qty) {
  static const  regex tail_re(R"((.*\S)\s+(\d{1,4})(?:\s|$))");
   smatch match;
  if (! regex_search(line, match, tail_re)) {
    return false;
  }
  name = trim(match[1].str());
  qty = match[2].str();
  if (contains(name, "(") || contains(name, "국내배송") || contains(name, "일반상품") ||
      contains(name, "해외배송") || contains(name, "CART") || contains(name, "LOGOUT")) {
    return false;
  }
  static const  regex korean_re(R"([가-힣])");
  return regex_search(name, korean_re);
}


string clean_product_text(const string& raw) {
  string line = normalize(raw);
  if (line.empty()) {
    return line;
  }
  string tail_name;
  string tail_qty;
  if (extract_qty_tail(line, tail_name, tail_qty)) {
    line = tail_name;
  }
  const size_t won = line.find(u8"원");
  if (won != string::npos) {
    line = trim(line.substr(0, won));
    const size_t last_space = line.find_last_of(' ');
    if (last_space != string::npos &&
        all_of(line.begin() + static_cast<ptrdiff_t>(last_space) + 1, line.end(),
               [](unsigned char ch) { return isdigit(ch) != 0 || ch == ','; })) {
      line = trim(line.substr(0, last_space));
    }
  }
  static const array<const char*, 3> kTrailingUi = {" 관심상품등록", " 변경", " 차등"};
  for (bool stripped = true; stripped;) {
    stripped = false;
    for (const char* suffix : kTrailingUi) {
      const size_t len = strlen(suffix);
      if (line.size() >= len && line.compare(line.size() - len, len, suffix) == 0) {
        line = trim(line.substr(0, line.size() - len));
        stripped = true;
        break;
      }
    }
  }
  return line;
}

bool is_product_line(const string& raw) {
  if (leading_spaces(raw) < 18) {
    return false;
  }
  const string line = clean_product_text(raw);
  return !line.empty() && !is_meta_line(line);
}

 vector< string> dedupe(const  vector< string>& parts) {
   vector< string>  output;
  for ( string part : parts) {
    static const  regex order_re(R"(\s*주문하기\s*$)");
    part =  regex_replace(part, order_re, "");
    part = trim(part);
    if (part.empty()) {
      continue;
    }
    if (! output.empty() &&  output.back() == part) {
      continue;
    }
     output.push_back(part);
  }
  return  output;
}

 string join_parts(const  vector< string>& parts) {
   ostringstream oss;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) {
      oss << " / ";
    }
    oss << parts[i];
  }
  return oss.str();
}

 vector< string> collect_block(const  vector< string>& lines,
                                       int start,
                                       int end,
                                        string& qty) {
  qty = "1";
  for (int i = start; i <= end; ++i) {
    const  string& raw = lines[i];
    if (extract_qty_only(raw, qty)) {
      continue;
    }
    const  string line = normalize(raw);
     string tail_name;
     string tail_qty;
    if (extract_qty_tail(line, tail_name, tail_qty)) {
      qty = tail_qty;
    }
  }

   vector< string> parts;
  for (int i = start; i <= end; ++i) {
    const  string& raw = lines[i];
    if (!extract_option(raw).empty()) {
      continue;
    }
    if (!is_product_line(raw)) {
      continue;
    }
    const string line = clean_product_text(raw);
    string tail_name;
    string tail_qty;
    if (extract_qty_tail(normalize(raw), tail_name, tail_qty)) {
      if (!is_meta_line(line)) {
        parts.push_back(line);
      }
      continue;
    }
    if (!is_meta_line(line)) {
      parts.push_back(line);
    }
  }
  return dedupe(parts);
}

int previous_option_index(const  vector< string>& lines, int option_index) {
  for (int i = option_index - 1; i >= 0; --i) {
    if (!extract_option(lines[i]).empty()) {
      return i;
    }
  }
  return -1;
}

 vector< string> collect_backward_item(const  vector< string>& lines,
                                               int option_index,
                                                string& qty) {
   vector< string> parts;
  bool found_qty = false;
  qty = "1";

  for (int i = option_index - 1; i >= 0; --i) {
    const  string& raw = lines[i];
    if (!extract_option(raw).empty()) {
      break;
    }
    if (found_qty && is_stop_backtrack_line(raw)) {
      break;
    }
    if (extract_qty_only(raw, qty)) {
      found_qty = true;
      continue;
    }

    const  string line = normalize(raw);
     string tail_name;
     string tail_qty;
    if (extract_qty_tail(line, tail_name, tail_qty)) {
      found_qty = true;
      qty = tail_qty;
      if (!is_meta_line(tail_name)) {
        parts.insert(parts.begin(), tail_name);
      }
      continue;
    }

    if (!found_qty || !is_product_line(raw) || is_meta_line(line)) {
      continue;
    }
    parts.insert(parts.begin(), line);
  }

  return dedupe(parts);
}

CartItem parse_option_item(const  vector< string>& lines, int option_index) {
  const  string option = extract_option(lines[option_index]);
  const int prev_option = previous_option_index(lines, option_index);
  const int start = prev_option + 1;

  int footer = -1;
  for (int i = option_index - 1; i > prev_option; --i) {
    if (contains(lines[i], "[기본배송]")) {
      footer = i;
      break;
    }
  }

   string qty = "1";
   vector< string> parts;
  const bool footer_in_range = footer != -1 && footer < option_index;
  if (footer_in_range) {
    parts = collect_block(lines, start, footer - 1, qty);
  } else {
    parts = collect_backward_item(lines, option_index, qty);
  }

  CartItem item;
  item.quantity = qty.empty() ? "1" : qty;
  item.product = join_parts(parts);
  item.option = option;
  item.price = "";
  const int price_end = footer_in_range ? footer - 1 : option_index - 1;
  for (int i = start; i <= price_end; ++i) {
    const string p = extract_price(lines[i]);
    if (!p.empty()) {
      item.price = p;
    }
  }
  if (footer_in_range && footer < option_index - 1) {
    for (int i = footer + 1; i <= option_index - 1; ++i) {
      const string p = extract_price(lines[i]);
      if (!p.empty()) {
        item.price = p;
      }
    }
  }
  return item;
}

 vector<CartItem> parse_cart_text(const  string& text) {
   vector< string> lines;
   istringstream input(text);
   string line;
  while ( getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(line);
  }

   vector<CartItem> items;
  for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
    if (extract_option(lines[i]).empty()) {
      continue;
    }
    items.push_back(parse_option_item(lines, i));
  }

  for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
    if (normalize(lines[i]) != "양말 낱개포장") {
      continue;
    }
     string qty = "1";
     vector< string> parts = {"양말 낱개포장"};
    int block_end = min(i + 8, static_cast<int>(lines.size())) - 1;
    for (int j = i + 1; j <= block_end; ++j) {
      extract_qty_only(lines[j], qty);
      const  string n = normalize(lines[j]);
      if (n == "남성용") {
        parts.push_back(n);
      }
      if (!extract_option(lines[j]).empty()) {
        block_end = j - 1;
        break;
      }
    }
    string price;
    for (int j = i; j <= block_end; ++j) {
      const string found = extract_price(lines[j]);
      if (!found.empty()) {
        price = found;
      }
    }
    if (items.size() >= 1) {
      items.insert(items.begin() + 1, {join_parts(parts), "", qty, price});
    } else {
      items.push_back({join_parts(parts), "", qty, price});
    }
    break;
  }

  return items;
}

 string shell_quote(const  string& value) {
   string quoted = "'";
  for (char ch : value) {
    if (ch == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(ch);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

 string find_pdftotext() {
  const char* env =  getenv("PDFTOTEXT");
  if (env != nullptr && *env != '\0') {
    return env;
  }
  const  array<const char*, 3> candidates = {
      "/opt/homebrew/bin/pdftotext",
      "/usr/local/bin/pdftotext",
      "pdftotext",
  };
  for (const char* candidate : candidates) {
    const  string command =  string(candidate) + " -v >/dev/null 2>&1";
    if ( system(command.c_str()) == 0) {
      return candidate;
    }
  }
  return "";
}

 string extract_pdf_text(const  string& pdf_path,  string& error) {
  const  string pdftotext = find_pdftotext();
  if (pdftotext.empty()) {
    error = "pdftotext not found. Install with: brew install poppler";
    return "";
  }

  const  string command =
      shell_quote(pdftotext) + " -layout " + shell_quote(pdf_path) + " -";
  FILE* pipe = popen(command.c_str(), "r");
  if (pipe == nullptr) {
    error = "Failed to run pdftotext";
    return "";
  }

   ostringstream output;
   array<char, 4096> buffer{};
  while (true) {
    const size_t bytes =  fread(buffer.data(), 1, buffer.size(), pipe);
    if (bytes == 0) {
      break;
    }
    output.write(buffer.data(), static_cast< streamsize>(bytes));
  }
  const int status = pclose(pipe);
  if (status != 0) {
    error = "pdftotext failed while reading " + pdf_path;
    return "";
  }
  return output.str();
}

 string xml_escape(const  string& value) {
   string  output;
   output.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '&':
         output += "&amp;";
        break;
      case '<':
         output += "&lt;";
        break;
      case '>':
         output += "&gt;";
        break;
      case '"':
         output += "&quot;";
        break;
      default:
         output.push_back(ch);
        break;
    }
  }
  return  output;
}

bool write_excel(const  string& output_path,
                 const  vector<CartItem>& items,
                  string& error) {
   ofstream  output(output_path,  ios::binary);
  if (! output) {
    error = "Could not open output file: " + output_path;
    return false;
  }

   output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
   output << "<?mso-application progid=\"Excel.Sheet\"?>\n";
   output << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" "
         "xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\">\n";
   output << " <Worksheet ss:Name=\"Cart\">\n";
   output << "  <Table>\n";
   output << "   <Row><Cell><Data ss:Type=\"String\">"
      << xml_escape("상품정보") << "</Data></Cell>";
   output << "<Cell><Data ss:Type=\"String\">"
      << xml_escape("옵션") << "</Data></Cell>";
  output << "<Cell><Data ss:Type=\"String\">"
      << xml_escape("수량") << "</Data></Cell>";
  output << "<Cell><Data ss:Type=\"String\">"
      << xml_escape("가격") << "</Data></Cell></Row>\n";

  for (const CartItem& item : items) {
    output << "   <Row><Cell><Data ss:Type=\"String\">"
        << xml_escape(item.product) << "</Data></Cell>";
    output << "<Cell><Data ss:Type=\"String\">"
        << xml_escape(item.option) << "</Data></Cell>";
    output << "<Cell><Data ss:Type=\"String\">"
        << xml_escape(item.quantity) << "</Data></Cell>";
    output << "<Cell><Data ss:Type=\"String\">"
        << xml_escape(item.price) << "</Data></Cell></Row>\n";
  }

   output << "  </Table>\n";
   output << " </Worksheet>\n";
   output << "</Workbook>\n";
  return true;
}

 string pdf_stem(const  string& pdf_path) {
  const size_t slash = pdf_path.find_last_of("/\\");
  const size_t dot = pdf_path.find_last_of('.');
  const size_t name_start = slash ==  string::npos ? 0 : slash + 1;
  if (dot !=  string::npos && dot > name_start) {
    return pdf_path.substr(name_start, dot - name_start);
  }
  return pdf_path.substr(name_start);
}

 string default_desktop_output_path(const  string& pdf_path) {
  const char* home =  getenv("HOME");
  const  string desktop =
      home != nullptr ?  string(home) + "/Desktop" : "~/Desktop";
  return desktop + "/" + pdf_stem(pdf_path) + "_cart.xls";
}

void print_usage(const char* program) {
   cerr << "Usage: " << program << " <cart.pdf> [output.xls]\n";
   cerr << "\n";
   cerr << "Example:\n";
   cerr << "  " << program << " ~/Downloads/Socks..pdf\n";
   cerr << "  " << program << " ~/Downloads/Socks..pdf ~/Desktop/cart.xls\n";
   cerr << "\nDefault output: ~/Desktop/<pdf_name>_cart.xls\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    print_usage(argv[0]);
    return 1;
  }

  const  string pdf_path = argv[1];
   string output_path;
  if (argc == 3) {
    output_path = argv[2];
  } else {
    output_path = default_desktop_output_path(pdf_path);
  }

   string error;
  const  string text = extract_pdf_text(pdf_path, error);
  if (text.empty()) {
     cerr << error << "\n";
    return 1;
  }

  const  vector<CartItem> items = parse_cart_text(text);
  if (items.empty()) {
     cerr << "No cart items found in PDF.\n";
    return 1;
  }

  if (!write_excel(output_path, items, error)) {
     cerr << error << "\n";
    return 1;
  }

   cout << "Saved " << items.size() << " item(s) to " << output_path << "\n";
  for (const CartItem& item : items) {
     cout << item.product << "\t" << item.option << "\t" << item.quantity << "\t"
         << item.price << "\n";
  }
  return 0;
}
