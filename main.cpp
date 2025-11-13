#include <iostream>
#include <Windows.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::streamoff FinalOffset = 0x0001E764;  // isLocked method offset in local file
const size_t bodySize = 111;               // isLocked method lenght: 0x6F
const std::vector<uint8_t> expectedPrefix = { 0x02, 0x7B, 0xBB, 0x05, 0x00, 0x04, 0x2C, 0x13, 0x28, 0x9A, 0x00, 0x00, 0x0A, 0x6F, 0xC7, 0x03, 0x00, 0x06, 0x02, 0x7B, 0xBC, 0x05, 0x00, 0x04, 0xFE, 0x04, 0x2A, 0x02, 0x7B, 0xB8, 0x05, 0x00, 0x04, 0x2D, 0x0A, 0x02, 0x7B, 0xBD, 0x05, 0x00, 0x04, 0x2D, 0x02, 0x16, 0x2A, 0x02, 0x7B, 0xBD, 0x05, 0x00, 0x04, 0x17, 0x33, 0x0E, 0x28, 0x9A, 0x00, 0x00, 0x0A, 0x6F, 0xC7, 0x03, 0x00, 0x06, 0x1E, 0xFE, 0x04, 0x2A, 0x02, 0x7B, 0xBD, 0x05, 0x00, 0x04, 0x18, 0x33, 0x0E, 0x28, 0x9A, 0x00, 0x00, 0x0A, 0x6F, 0xC8, 0x03, 0x00, 0x06, 0x16, 0xFE, 0x01, 0x2A, 0x28, 0x9A, 0x00, 0x00, 0x0A, 0x02, 0x7B, 0xB8, 0x05, 0x00, 0x04, 0x6F, 0xC9, 0x03, 0x00, 0x06, 0x16, 0xFE, 0x01, 0x2A };
const size_t SizeOfMethodToRead = expectedPrefix.size();

int main(int argc, char* argv[]) {
	std::string PeakDllFilePath = "";
	if (argc >= 2) {
		PeakDllFilePath = argv[1];
	} else {
		std::cout << "Please locate Assembly-CSharp.dll file in <...steamapps\\common\\PEAK\\PEAK_Data\\Managed> folder" << '\n';
		std::cout << "Next time u can just drag'n'drop dll file into this program, it'll patch it automatically" << '\n';
		std::cout << "Paste path to file: ";
		std::getline(std::cin, PeakDllFilePath);

		PeakDllFilePath.erase(
			std::remove(PeakDllFilePath.begin(), PeakDllFilePath.end(), '\"'),
			PeakDllFilePath.end());
	}
	
	std::cout << "[?] Entered path: " << PeakDllFilePath << " [?]" << '\n';
	Sleep(500);
	


	if (!fs::exists(PeakDllFilePath)) {
		std::cerr << "[-] Provided path DOES NOT contain a file [-]" << std::endl;
		system("pause");
		return 1;
	}

	std::vector<uint8_t> isLockedNewBody(bodySize, 0x00);
	isLockedNewBody[0] = 0x20;  // ldc.i4
	isLockedNewBody[1] = 0x00;
	isLockedNewBody[2] = 0x00;
	isLockedNewBody[3] = 0x00;
	isLockedNewBody[4] = 0x00;
	isLockedNewBody[5] = 0x2A;  // ret

	std::fstream PeakDllFile(PeakDllFilePath.c_str(), std::ios::binary | std::ios::in | std::ios::out);
	if (!PeakDllFile.is_open()) {
		std::cerr << "[-] Unable to open file: " << PeakDllFilePath << " with error: " << GetLastError() << " [-]" << std::endl;
		PeakDllFile.close();
		system("pause");
		return 2;
	}
	
	std::vector<uint8_t> ReadedBuffer(SizeOfMethodToRead);
	bool IsSignatureMatch = false;
	auto PeakDllFileSize = fs::file_size(PeakDllFilePath);
	std::cout << "[?] Assembly-Sharp.dll size: 0x" << std::hex << PeakDllFileSize << " [?]" << '\n';

	for (auto offset = 0; offset < PeakDllFileSize; offset++) {
		PeakDllFile.seekp(offset);
		PeakDllFile.read(reinterpret_cast<char*>(ReadedBuffer.data()), SizeOfMethodToRead);

		IsSignatureMatch = (ReadedBuffer.size() >= expectedPrefix.size());
		for (size_t i = 0; IsSignatureMatch && i < expectedPrefix.size(); i++) {
			if (ReadedBuffer[i] != expectedPrefix[i]) {
				IsSignatureMatch = false;
				break;
			}
		}
		if (!IsSignatureMatch) {
			continue;
		}
		for (uint8_t i = 0; i < SizeOfMethodToRead; i++) {
			if (ReadedBuffer[i] < 10) {
				std::cout << "0x0" << std::hex << (int)ReadedBuffer[i] << " ";
				continue;
			}
			std::cout << "0x" << std::hex << (int)ReadedBuffer[i] << " ";
			
		}
		std::cout << "\n[+] Found (mr.) proper bytes of isLocked method at offset: 0x" << std::hex << offset << " [+]" << '\n';
		FinalOffset = offset;
		break;
	}
	if (!IsSignatureMatch) {
		PeakDllFile.close();
		std::cerr << "[-] Unable to locate methods bytes [-]" << std::endl;
		system("pause");
		return 3;
	}


	PeakDllFile.seekp(FinalOffset);
	PeakDllFile.write(reinterpret_cast<const char *>(isLockedNewBody.data()), static_cast<std::streamsize>(isLockedNewBody.size()));

	if (!PeakDllFile.good()) {
		PeakDllFile.close();
		std::wcerr << "[-] Error on writing data, code: " << GetLastError() << " [-]" << std::endl;
		system("pause");
		return 4;
	}
	PeakDllFile.close();
	std::cout << "[+] Success in editing isLocked method, now all skins are open [+]" << std::endl;
	system("pause");
	return 0;
}