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
        if (index >= lines.size()) {
            std::cout << "Out of range!\n";
            return;
        }
        for (auto line : clipboard) delete line;
        clipboard.clear();
        clipboard.push_back(lines[index]->clone());
        std::cout << "Line copied to clipboard.\n";
    }

    void cutLine(size_t index) {
        if (index >= lines.size()) {
            std::cout << "Out of range!\n";
            return;
        }
        copyLine(index);
        delete lines[index];
        lines.erase(lines.begin() + index);
        std::cout << "Line cut to clipboard.\n";
    }

    void pasteLine(size_t index) {
        if (clipboard.empty()) {
            std::cout << "Clipboard is empty!\n";
            return;
        }
        if (index > lines.size()) index = lines.size();

        lines.insert(lines.begin() + index, clipboard[0]->clone());
        std::cout << "Line pasted.\n";
    }

    void toggleLine(size_t index) {
        if (index >= lines.size()) {
            std::cout << "Out of range!\n";
            return;
        }

        ChecklistLine* chk = dynamic_cast<ChecklistLine*>(lines[index]);

        if (chk != nullptr) {
            chk->toggle();
            std::cout << "Task status updated!\n";
        }
        else {
            std::cout << "Error: This line is not a checklist task!\n";
        }
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
            std::cout << "5. Copy Line\n6. Cut Line\n7. Paste Line\n8. Save Encrypted\n9.Toogle checklist\n0. Exit\n> ";

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
