#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <fstream>

typedef void* cipher_t;
typedef cipher_t(*CreateCaesarFn)(int);
typedef cipher_t(*CreateVigenereFn)(const char*);
typedef char* (*CipherEncryptFn)(cipher_t, const char*);
typedef char* (*CipherDecryptFn)(cipher_t, const char*);
typedef void (*CipherDestroyFn)(cipher_t);
typedef void (*CipherFreeFn)(char*);

class Line {
public:
    virtual void print() const = 0;
    virtual std::string serialize() const = 0;
    virtual Line* clone() const = 0;

    virtual std::string getRawText() const = 0;
    virtual void setRawText(const std::string& new_text) = 0;

    virtual ~Line() {}
};

class TextLine : public Line {
private:
    std::string text;
public:
    TextLine(const std::string& t) : text(t) {}

    void print() const override {
        std::cout << "Text: " << text << std::endl;
    }
    std::string serialize() const override {
        return "T|" + text;
    }
    Line* clone() const override {
        return new TextLine(text);
    }

    std::string getRawText() const override { return text; }
    void setRawText(const std::string& new_text) override { text = new_text; }
};

class ChecklistLine : public Line {
private:
    std::string item;
    bool checked;
public:
    ChecklistLine(const std::string& i, bool c) : item(i), checked(c) {}

    void toggle() {
        checked = !checked;
    }

    void print() const override {
        std::cout << "[ " << (checked ? "x" : " ") << " ] " << item << std::endl;
    }
    std::string serialize() const override {
        return "C|" + std::string(checked ? "1" : "0") + "|" + item;
    }
    Line* clone() const override {
        return new ChecklistLine(item, checked);
    }

    std::string getRawText() const override { return item; }
    void setRawText(const std::string& new_text) override { item = new_text; }
};

class ContactLine : public Line {
private:
    std::string name;
    std::string surname;
    std::string email;
public:
    ContactLine(const std::string& n, const std::string& s, const std::string& e)
        : name(n), surname(s), email(e) {
    }
    void print() const override {
        std::cout << "Contact - " << name << " " << surname << ", E-mail: " << email << std::endl;
    }
    std::string serialize() const override {
        return "P|" + name + "|" + surname + "|" + email;
    }
    Line* clone() const override {
        return new ContactLine(name, surname, email);
    }

    std::string getRawText() const override { return name; }
    void setRawText(const std::string& new_text) override { name = new_text; }
};

class CipherManager {
public:
    static std::string processCaesar(int key, const std::string& text, bool encrypt) {
        HMODULE hLib = LoadLibrary(L"CipherLib.dll");
        if (!hLib) {
            std::cout << "ERROR: Cannot load CipherLib.dll\n";
            return text;
        }

        CreateCaesarFn create = (CreateCaesarFn)GetProcAddress(hLib, "cipher_create_caesar");
        CipherEncryptFn encFn = (CipherEncryptFn)GetProcAddress(hLib, "cipher_encrypt");
        CipherDecryptFn decFn = (CipherDecryptFn)GetProcAddress(hLib, "cipher_decrypt");
        CipherDestroyFn destroy = (CipherDestroyFn)GetProcAddress(hLib, "cipher_destroy");
        CipherFreeFn freeStr = (CipherFreeFn)GetProcAddress(hLib, "cipher_free");

        cipher_t cipher = create(key);
        char* res = encrypt ? encFn(cipher, text.c_str()) : decFn(cipher, text.c_str());

        std::string result(res);

        freeStr(res);
        destroy(cipher);
        FreeLibrary(hLib);

        return result;
    }
};

class TextDocument {
private:
    std::vector<Line*> lines;
    std::vector<Line*> clipboard;
    std::string char_clipboard;

public:
    ~TextDocument() { clear(); }

    void addLine(Line* line) {
        lines.push_back(line);
    }

    void printAll() const {
        if (lines.empty()) {
            std::cout << "(Document is empty)\n";
            return;
        }
        for (size_t i = 0; i < lines.size(); ++i) {
            std::cout << i << ": ";
            lines[i]->print();
        }
    }


    void copyLine(size_t index) {
        if (index >= lines.size()) { std::cout << "Out of range!\n"; return; }
        for (auto line : clipboard) delete line;
        clipboard.clear();
        clipboard.push_back(lines[index]->clone());
        std::cout << "Line copied to clipboard.\n";
    }

    void cutLine(size_t index) {
        if (index >= lines.size()) { std::cout << "Out of range!\n"; return; }
        copyLine(index);
        delete lines[index];
        lines.erase(lines.begin() + index);
        std::cout << "Line cut to clipboard.\n";
    }

    void pasteLine(size_t index) {
        if (clipboard.empty()) { std::cout << "Clipboard is empty!\n"; return; }
        if (index > lines.size()) index = lines.size();
        lines.insert(lines.begin() + index, clipboard[0]->clone());
        std::cout << "Line pasted.\n";
    }

    void toggleLine(size_t index) {
        if (index >= lines.size()) { std::cout << "Out of range!\n"; return; }
        ChecklistLine* chk = dynamic_cast<ChecklistLine*>(lines[index]);
        if (chk != nullptr) {
            chk->toggle();
            std::cout << "Task status updated!\n";
        }
        else {
            std::cout << "Error: This line is not a checklist task!\n";
        }
    }

    void insertText(size_t target_line, size_t target_char_idx, const std::string& text_to_insert) {
        if (target_line >= lines.size()) { std::cout << "Out of range!\n"; return; }
        std::string current_text = lines[target_line]->getRawText();
        if (target_char_idx > current_text.length()) target_char_idx = current_text.length();
        current_text.insert(target_char_idx, text_to_insert);
        lines[target_line]->setRawText(current_text);
        std::cout << "Text inserted.\n";
    }

    void copyText(size_t target_line, size_t target_char_idx, size_t num_of_symbols) {
        if (target_line >= lines.size()) { std::cout << "Out of range!\n"; return; }
        std::string current_text = lines[target_line]->getRawText();
        if (target_char_idx >= current_text.length()) { std::cout << "Index too large.\n"; return; }
        if (target_char_idx + num_of_symbols > current_text.length()) {
            num_of_symbols = current_text.length() - target_char_idx;
        }
        char_clipboard = current_text.substr(target_char_idx, num_of_symbols);
        std::cout << "Characters copied.\n";
    }

    void cutText(size_t target_line, size_t target_char_idx, size_t num_of_symbols) {
        if (target_line >= lines.size()) { std::cout << "Out of range!\n"; return; }
        std::string current_text = lines[target_line]->getRawText();
        if (target_char_idx >= current_text.length()) return;
        if (target_char_idx + num_of_symbols > current_text.length()) {
            num_of_symbols = current_text.length() - target_char_idx;
        }
        char_clipboard = current_text.substr(target_char_idx, num_of_symbols);
        current_text.erase(target_char_idx, num_of_symbols);
        lines[target_line]->setRawText(current_text);
        std::cout << "Characters cut.\n";
    }

    void pasteText(size_t target_line, size_t target_char_idx) {
        if (char_clipboard.empty()) { std::cout << "Char clipboard is empty!\n"; return; }
        if (target_line >= lines.size()) { std::cout << "Out of range!\n"; return; }
        std::string current_text = lines[target_line]->getRawText();
        if (target_char_idx > current_text.length()) target_char_idx = current_text.length();
        current_text.insert(target_char_idx, char_clipboard);
        lines[target_line]->setRawText(current_text);
        std::cout << "Characters pasted.\n";
    }

    void insertWithReplacement(size_t target_line, size_t target_char_idx, const std::string& text_to_insert) {
        if (target_line >= lines.size()) { std::cout << "Out of range!\n"; return; }
        std::string current_text = lines[target_line]->getRawText();
        if (target_char_idx > current_text.length()) target_char_idx = current_text.length();
        size_t chars_to_replace = text_to_insert.length();
        if (target_char_idx + chars_to_replace > current_text.length()) {
            chars_to_replace = current_text.length() - target_char_idx;
        }
        current_text.replace(target_char_idx, chars_to_replace, text_to_insert);
        lines[target_line]->setRawText(current_text);
        std::cout << "Text inserted with replacement.\n";
    }

    void saveEncrypted(const std::string& filename, int key) {
        std::ofstream out(filename);
        if (!out) {
            std::cout << "Cannot open file for writing.\n";
            return;
        }
        for (const auto& line : lines) {
            std::string serialized = line->serialize();
            std::string encrypted = CipherManager::processCaesar(key, serialized, true);
            out << encrypted << "\n";
        }
        std::cout << "Document saved and encrypted successfully!\n";
    }

    void clear() {
        for (auto line : lines) delete line;
        lines.clear();
        for (auto line : clipboard) delete line;
        clipboard.clear();
    }
};

class CommandLineInterface {
private:
    TextDocument doc;

public:
    void run() {
        while (true) {
            std::cout << "\n--- MENU ---\n";
            std::cout << "1. Add Text\n2. Add Contact\n3. Add Checklist\n4. Print Document\n";
            std::cout << "5. Copy Line\n6. Cut Line\n7. Paste Line\n8. Save Encrypted\n9. Toggle checklist\n";
            std::cout << "--- CHAR OPERATIONS ---\n";
            std::cout << "10. Insert Chars\n11. Copy Chars\n12. Cut Chars\n13. Paste Chars\n14. Replace Chars\n";
            std::cout << "0. Exit\n> ";

            int choice;
            std::cin >> choice;

            if (std::cin.fail()) {
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                continue;
            }

            switch (choice) {
            case 1: {
                std::string text;
                std::cout << "Enter text: ";
                std::cin.ignore();
                std::getline(std::cin, text);
                doc.addLine(new TextLine(text));
                break;
            }
            case 2: {
                std::string name, surname, email;
                std::cout << "Name: "; std::cin >> name;
                std::cout << "Surname: "; std::cin >> surname;
                std::cout << "Email: "; std::cin >> email;
                doc.addLine(new ContactLine(name, surname, email));
                break;
            }
            case 3: {
                std::string item;
                int checked_int;
                std::cout << "Task description: ";
                std::cin.ignore();
                std::getline(std::cin, item);
                std::cout << "Is it done? (1 - Yes, 0 - No): ";
                std::cin >> checked_int;
                doc.addLine(new ChecklistLine(item, checked_int != 0));
                break;
            }
            case 4:
                std::cout << "\n--- CURRENT DOCUMENT ---\n";
                doc.printAll();
                break;
            case 5: {
                size_t idx;
                std::cout << "Enter line index to copy: ";
                std::cin >> idx;
                doc.copyLine(idx);
                break;
            }
            case 6: {
                size_t idx;
                std::cout << "Enter line index to cut: ";
                std::cin >> idx;
                doc.cutLine(idx);
                break;
            }
            case 7: {
                size_t idx;
                std::cout << "Enter index to paste AT: ";
                std::cin >> idx;
                doc.pasteLine(idx);
                break;
            }
            case 8: {
                std::string filename;
                int key;
                std::cout << "Enter filename (e.g., test.txt): ";
                std::cin >> filename;
                std::cout << "Enter Caesar key (e.g., 3): ";
                std::cin >> key;
                doc.saveEncrypted(filename, key);
                break;
            }
            case 9: {
                size_t idx;
                std::cout << "Enter line index to mark as done/undone: ";
                std::cin >> idx;
                doc.toggleLine(idx);
                break;
            }

            case 10: {
                size_t line_idx, char_idx;
                std::string text;
                std::cout << "Enter line index and char index: ";
                std::cin >> line_idx >> char_idx;
                std::cout << "Enter text to insert: ";
                std::cin.ignore();
                std::getline(std::cin, text);
                doc.insertText(line_idx, char_idx, text);
                break;
            }
            case 11: { // Copy
                size_t line_idx, char_idx, num;
                std::cout << "Enter line index, char index and number of symbols: ";
                std::cin >> line_idx >> char_idx >> num;
                doc.copyText(line_idx, char_idx, num);
                break;
            }
            case 12: { // Cut
                size_t line_idx, char_idx, num;
                std::cout << "Enter line index, char index and number of symbols: ";
                std::cin >> line_idx >> char_idx >> num;
                doc.cutText(line_idx, char_idx, num);
                break;
            }
            case 13: { // Paste
                size_t line_idx, char_idx;
                std::cout << "Enter line index and char index: ";
                std::cin >> line_idx >> char_idx;
                doc.pasteText(line_idx, char_idx);
                break;
            }
            case 14: { // Replace
                size_t line_idx, char_idx;
                std::string text;
                std::cout << "Enter line index and char index: ";
                std::cin >> line_idx >> char_idx;
                std::cout << "Enter text to replace with: ";
                std::cin.ignore();
                std::getline(std::cin, text);
                doc.insertWithReplacement(line_idx, char_idx, text);
                break;
            }
            case 0:
                std::cout << "Exiting...\n";
                return;
            default:
                std::cout << "Unknown command.\n";
            }
        }
    }
};

int main() {
    CommandLineInterface cli;
    cli.run();
    return 0;
}
