/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2023 Christo Joseph.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <limits.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

static const char g_file_separator = '/';

static inline void json_item_substitute_file_path_symlink_path(nlohmann::json& item, const std::string realpath_sym,
															   const std::string& symlink_fullpath) {
	const std::string& file = item["file"];
	const std::string& directory = item["directory"];
	const std::string full_path = directory + g_file_separator + file;
	char realpath_current[PATH_MAX + 1];
	if (NULL == realpath(full_path.c_str(), realpath_current)) {
		std::cerr << "realpath() error. full_path : " << full_path << std::endl;
	} else {
		realpath_current[PATH_MAX - 1] = 0;
		std::string realpath_current_str{realpath_current};
		if (realpath_current_str.rfind(realpath_sym, 0) == 0) {
			item["file"] = symlink_fullpath + g_file_separator + realpath_current_str.substr(realpath_sym.length());
		} else {
			std::cerr << "No match. full_path : " << full_path << " realpath_current_str : " << realpath_current_str
					  << std::endl;
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc < 4) {
		std::cerr << "Usage : " << argv[0] << " <input-json-file> <output-json-file> <path of symlink>" << std::endl;
		return -1;
	}
	const std::string input = argv[1];
	const std::string output = argv[2];

	const char* symlink_cstr = argv[3];
	size_t symlink_start = 0;
	size_t symlink_end = strlen(symlink_cstr);
	if (symlink_cstr[0] == '.' && symlink_cstr[1] == '/') {
		symlink_start = 2;
	}
	if (symlink_cstr[symlink_end - 1] == '/') {
		symlink_end--;
	}
	std::string symlink{argv[3], symlink_start, symlink_end};

	std::ifstream inputFile(input);
	if (!inputFile.good()) {
		std::cerr << "Could not open json file : " << input << std::endl;
		return -1;
	}

	nlohmann::json json;
	inputFile >> json;

	std::string symlink_fullpath;
	if (symlink.front() != g_file_separator) {
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			std::cerr << "getcwd() error" << std::endl;
			return -1;
		}
		symlink_fullpath = std::string(cwd) + g_file_separator + symlink;
	} else {
		symlink_fullpath = symlink;
	}
	std::cout << "symlink_fullpath : " << symlink_fullpath << std::endl;

	char realpath_sym[PATH_MAX];
	int realpath_sym_len = readlink(symlink_fullpath.c_str(), realpath_sym, sizeof(realpath_sym));
	if (-1 == realpath_sym_len) {
		std::cerr << "readlink() error" << std::endl;
		return -1;
	}
	realpath_sym[realpath_sym_len] = 0;

	for (nlohmann::json& item : json) {
		// std::cout << std::setw(4) << item << std::endl << std::endl;
		const std::string& file = item["file"];
		if (file.rfind(realpath_sym, 0) == 0) {
			item["file"] = symlink_fullpath + g_file_separator + file.substr(realpath_sym_len);
		} else if (file.at(0) != g_file_separator) {
			json_item_substitute_file_path_symlink_path(item, realpath_sym, symlink_fullpath);
		} else {
			std::cerr << "No match. file : " << file << " realpath_sym : " << realpath_sym << std::endl;
		}
		// std::cout << std::setw(4) << item << std::endl << std::endl;
	}

	std::ofstream out_file(output);
	out_file << std::setw(4) << json << std::endl;
	return 0;
}
