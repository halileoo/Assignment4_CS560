#include "pch.h"
#include "cipher_api.h"
#include <string>
#include <vector>
#include <iostream>

std::vector<char> az = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
				'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

std::vector<char> AZ = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
				'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };

int find_by_value(std::vector<char>& arr, char value) {
	for (int i = 0; i < arr.size(); ++i) {
		if (arr[i] == value) {
			return i;
		}
	}
	return -1;
}

class Cipher {
public:
	virtual std::string encrypt(const std::string& text) = 0;
	virtual std::string decrypt(const std::string& text) = 0;
	virtual ~Cipher() = default;
};

class CaesarCipher : public Cipher {
	int key_ = 0;
public:
	CaesarCipher(int key) {
		key_ = key;
	}

	std::string encrypt(const std::string& text) override {
		std::string res_text;

		for (int i = 0; i < text.length(); i++) {
			int low_idx = find_by_value(az, text[i]);
			int upp_idx = find_by_value(AZ, text[i]);

			if (low_idx != -1) {
				int shifted = (low_idx + key_) % (int)az.size();
				if (shifted < 0) shifted += az.size();

				res_text += az[shifted];
			}
			else if (upp_idx != -1) {
				int shifted = (upp_idx + key_) % (int)AZ.size();
				if (shifted < 0) shifted += AZ.size();

				res_text += AZ[shifted];
			}
			else {
				res_text += text[i];
			}
		}

		return res_text;
	}
	std::string decrypt(const std::string& text) override {
		CaesarCipher inverseCaesar(-key_);
		return inverseCaesar.encrypt(text);
	};
};

class VigenereCipher : public Cipher {
	std::string key_;
public:
	VigenereCipher(const std::string& key) {
		key_ = key;
	}

	std::string encrypt(const std::string& text) override {
		std::string res_text;
		int key_idx = 0;

		for (int i = 0; i < text.length(); i++) {
			int low_idx = find_by_value(az, text[i]);
			int upp_idx = find_by_value(AZ, text[i]);

			if (low_idx != -1 || upp_idx != -1) {
				char k_char = key_[key_idx % key_.length()];

				int shift = find_by_value(az, k_char);
				if (shift == -1) {
					shift = find_by_value(AZ, k_char);
				}

				if (low_idx != -1) {
					int shifted = (low_idx + shift) % (int)az.size();
					if (shifted < 0) shifted += az.size();
					res_text += az[shifted];
				}
				else if (upp_idx != -1) {
					int shifted = (upp_idx + shift) % (int)AZ.size();
					if (shifted < 0) shifted += AZ.size();
					res_text += AZ[shifted];
				}

				key_idx++;
			}
			else {
				res_text += text[i];
			}
		}
		return res_text;
	}

	std::string decrypt(const std::string& text) override {
		std::string res_text;
		int key_idx = 0;

		for (int i = 0; i < text.length(); i++) {
			int low_idx = find_by_value(az, text[i]);
			int upp_idx = find_by_value(AZ, text[i]);

			if (low_idx != -1 || upp_idx != -1) {
				char k_char = key_[key_idx % key_.length()];

				int shift = find_by_value(az, k_char);
				if (shift == -1) {
					shift = find_by_value(AZ, k_char);
				}

				if (low_idx != -1) {
					int shifted = (low_idx - shift) % (int)az.size();
					if (shifted < 0) shifted += az.size();
					res_text += az[shifted];
				}
				else if (upp_idx != -1) {
					int shifted = (upp_idx - shift) % (int)AZ.size();
					if (shifted < 0) shifted += AZ.size();
					res_text += AZ[shifted];
				}

				key_idx++;
			}
			else {
				res_text += text[i];
			}
		}
		return res_text;
	}
};

extern "C" {
	EXPORT cipher_t cipher_create_caesar(int key) {
		return static_cast<cipher_t>(new CaesarCipher(key));
	}

	EXPORT cipher_t cipher_create_vigenere(const char* key) {
		if (!key) return nullptr;
		return static_cast<cipher_t>(new VigenereCipher(std::string(key)));
	}

	EXPORT char* cipher_encrypt(cipher_t cipher, const char* text) {
		if (!cipher || !text) return nullptr;

		Cipher* obj = static_cast<Cipher*>(cipher);
		std::string cpp_res = obj->encrypt(std::string(text));

		char* c_res = new char[cpp_res.length() + 1];
		strcpy_s(c_res, cpp_res.length() + 1, cpp_res.c_str());

		return c_res;
	}

	EXPORT char* cipher_decrypt(cipher_t cipher, const char* text) {
		if (!cipher || !text) return nullptr;

		Cipher* obj = static_cast<Cipher*>(cipher);
		std::string cpp_res = obj->decrypt(std::string(text));

		char* c_res = new char[cpp_res.length() + 1];
		strcpy_s(c_res, cpp_res.length() + 1, cpp_res.c_str());

		return c_res;
	}

	EXPORT void cipher_destroy(cipher_t cipher) {
		if (cipher) {
			delete static_cast<Cipher*>(cipher);
		}
	}

	EXPORT void cipher_free(char* str) {
		if (str) {
			delete[] str;
		}
	}
}
