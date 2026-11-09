#include "Updater.hpp"

#include <algorithm>
#include <array>
#include <bcrypt.h>
#include <charconv>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cwchar>
#include <shellapi.h>
#include <sstream>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace {
constexpr std::string_view RELEASE_DOWNLOAD_PREFIX =
	"https://github.com/mariocrazyy/excel-strike/releases/download/";

std::wstring QuoteArgument(const std::wstring& argument) {
	return L"\"" + argument + L"\"";
}

bool GetExecutablePath(std::filesystem::path& path) {
	std::wstring buffer(32768, L'\0');
	DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	if (length == 0 || length >= buffer.size())
		return false;

	buffer.resize(length);
	path = std::filesystem::path(buffer);
	return true;
}

bool ParseVersion(std::string_view text, std::array<std::uint64_t, 3>& version) {
	if (text.size() < 6 || text[0] != 'v')
		return false;

	text.remove_prefix(1);
	for (size_t component = 0; component < version.size(); ++component) {
		const size_t separator = text.find('.');
		const std::string_view part = component == version.size() - 1
			? text
			: text.substr(0, separator);
		if (part.empty() || (component != version.size() - 1 && separator == std::string_view::npos))
			return false;

		const auto result = std::from_chars(part.data(), part.data() + part.size(), version[component]);
		if (result.ec != std::errc{} || result.ptr != part.data() + part.size())
			return false;

		if (component != version.size() - 1)
			text.remove_prefix(separator + 1);
	}

	return text.find('.') == std::string_view::npos;
}

int CompareVersions(std::string_view candidate, std::string_view current) {
	std::array<std::uint64_t, 3> candidate_parts{};
	std::array<std::uint64_t, 3> current_parts{};
	if (!ParseVersion(candidate, candidate_parts) || !ParseVersion(current, current_parts))
		return 0;

	if (candidate_parts < current_parts)
		return -1;
	if (candidate_parts > current_parts)
		return 1;
	return 0;
}

bool IsHexDigest(std::string_view value) {
	return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
		return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
	});
}

bool ParseChecksum(std::string_view content, std::string& digest) {
	while (!content.empty() && std::isspace(static_cast<unsigned char>(content.front())))
		content.remove_prefix(1);
	while (!content.empty() && std::isspace(static_cast<unsigned char>(content.back())))
		content.remove_suffix(1);

	const size_t separator = content.find_first_of(" \t\r\n");
	const std::string_view hash = content.substr(0, separator);
	if (!IsHexDigest(hash))
		return false;

	if (separator != std::string_view::npos) {
		content.remove_prefix(separator);
		while (!content.empty() && std::isspace(static_cast<unsigned char>(content.front())))
			content.remove_prefix(1);
		if (!content.empty() && content.front() == '*')
			content.remove_prefix(1);
		if (!content.empty() && content != "excel.exe")
			return false;
	}

	digest.assign(hash);
	return true;
}

bool VerifySha256(const std::filesystem::path& path, const std::string& expected) {
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	DWORD object_length = 0;
	DWORD hash_length = 0;
	DWORD result_length = 0;
	std::vector<UCHAR> hash_object;
	std::vector<UCHAR> hash_value;
	std::ifstream file(path, std::ios::binary);

	if (!file.is_open() ||
		BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
		return false;

	bool valid = BCryptGetProperty(
		algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_length),
		sizeof(object_length), &result_length, 0) >= 0 &&
		BCryptGetProperty(
			algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hash_length),
			sizeof(hash_length), &result_length, 0) >= 0;

	if (valid) {
		hash_object.resize(object_length);
		hash_value.resize(hash_length);
		valid = BCryptCreateHash(
			algorithm, &hash, hash_object.data(), object_length, nullptr, 0, 0) >= 0;
	}

	std::array<UCHAR, 65536> buffer{};
	while (valid) {
		file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
		const std::streamsize bytes_read = file.gcount();
		if (bytes_read > 0) {
			valid = BCryptHashData(hash, buffer.data(), static_cast<ULONG>(bytes_read), 0) >= 0;
		}
		if (bytes_read == 0)
			break;
	}

	if (valid)
		valid = file.eof() && BCryptFinishHash(hash, hash_value.data(), hash_length, 0) >= 0;

	if (hash)
		BCryptDestroyHash(hash);
	BCryptCloseAlgorithmProvider(algorithm, 0);

	if (!valid)
		return false;

	static constexpr char hex[] = "0123456789abcdef";
	std::string actual;
	actual.reserve(hash_value.size() * 2);
	for (UCHAR byte : hash_value) {
		actual.push_back(hex[byte >> 4]);
		actual.push_back(hex[byte & 0x0f]);
	}

	return _stricmp(actual.c_str(), expected.c_str()) == 0;
}

bool IsExpectedExecutable(const std::filesystem::path& path, DWORD expected_type) {
	DWORD binary_type = 0;
	return GetBinaryTypeW(path.c_str(), &binary_type) && binary_type == expected_type;
}

int ShowUpdateFailure(const wchar_t* message) {
	MessageBoxW(nullptr, message, L"Update Failed", MB_OK | MB_ICONERROR);
	return 1;
}

bool IsEmojiCodePoint(std::uint32_t code_point) {
	return (code_point >= 0x1F000 && code_point <= 0x1FAFF) ||
		(code_point >= 0x2600 && code_point <= 0x27BF) ||
		(code_point >= 0x1F1E6 && code_point <= 0x1F1FF) ||
		(code_point >= 0x1F3FB && code_point <= 0x1F3FF) ||
		code_point == 0xFE0F || code_point == 0x200D;
}

std::string RemoveEmoji(std::string_view text) {
	std::string result;
	for (size_t i = 0; i < text.size();) {
		const unsigned char first = static_cast<unsigned char>(text[i]);
		if (first < 0x80) {
			result.push_back(static_cast<char>(first));
			++i;
			continue;
		}

		size_t length = 0;
		std::uint32_t code_point = 0;
		if ((first & 0xE0) == 0xC0) {
			length = 2;
			code_point = first & 0x1F;
		}
		else if ((first & 0xF0) == 0xE0) {
			length = 3;
			code_point = first & 0x0F;
		}
		else if ((first & 0xF8) == 0xF0) {
			length = 4;
			code_point = first & 0x07;
		}
		else {
			++i;
			continue;
		}

		if (i + length > text.size()) {
			break;
		}
		for (size_t j = 1; j < length; ++j) {
			const unsigned char next = static_cast<unsigned char>(text[i + j]);
			if ((next & 0xC0) != 0x80) {
				length = 0;
				break;
			}
			code_point = (code_point << 6) | (next & 0x3F);
		}
		if (length == 0) {
			++i;
			continue;
		}

		if (!IsEmojiCodePoint(code_point))
			result.append(text.substr(i, length));
		i += length;
	}
	return result;
}

std::string StripMarkdown(std::string_view text) {
	std::string result;
	for (size_t i = 0; i < text.size();) {
		if (text[i] == '[') {
			const size_t label_end = text.find(']', i + 1);
			if (label_end != std::string_view::npos && label_end + 1 < text.size() &&
				text[label_end + 1] == '(') {
				result.append(text.substr(i + 1, label_end - i - 1));
				const size_t link_end = text.find(')', label_end + 2);
				i = link_end == std::string_view::npos ? text.size() : link_end + 1;
				continue;
			}
		}
		if (text[i] == '*' || text[i] == '_' || text[i] == '`' || text[i] == '~') {
			++i;
			continue;
		}
		result.push_back(text[i++]);
	}
	return result;
}

void FormatReleaseNotes(std::string_view markdown, std::string& title, std::string& body) {
	std::istringstream lines(RemoveEmoji(markdown));
	std::string line;
	while (std::getline(lines, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		const size_t first = line.find_first_not_of(" \t");
		if (first == std::string::npos) {
			if (!body.empty() && body.back() != '\n')
				body.push_back('\n');
			continue;
		}

		std::string_view content(line.data() + first, line.size() - first);
		size_t heading_end = 0;
		while (heading_end < content.size() && content[heading_end] == '#')
			++heading_end;
		if (heading_end > 0 && heading_end < content.size() && content[heading_end] == ' ')
			content.remove_prefix(heading_end + 1);
		else if (content.starts_with("- ") || content.starts_with("* ") || content.starts_with("+ "))
			content.remove_prefix(2);

		std::string formatted = StripMarkdown(content);
		const size_t formatted_first = formatted.find_first_not_of(" \t");
		const size_t formatted_last = formatted.find_last_not_of(" \t");
		if (formatted_first == std::string::npos)
			continue;
		formatted = formatted.substr(formatted_first, formatted_last - formatted_first + 1);

		if (heading_end > 0 && title.empty()) {
			title = std::move(formatted);
			continue;
		}

		if (content.data() != nullptr && (line.compare(first, 2, "- ") == 0 ||
			line.compare(first, 2, "* ") == 0 || line.compare(first, 2, "+ ") == 0))
			body += "- ";
		body += formatted;
		body.push_back('\n');
		if (body.size() >= 4096) {
			body.resize(4096);
			break;
		}
	}
	while (!body.empty() && (body.back() == '\n' || body.back() == ' '))
		body.pop_back();
}

int ApplyUpdate(DWORD parent_pid, const std::filesystem::path& target,
	const std::filesystem::path& staged) {
	std::filesystem::path helper;
	if (!GetExecutablePath(helper))
		return ShowUpdateFailure(L"Could not determine the updater executable path.");

	const std::filesystem::path expected_helper =
		target.parent_path() / L"OfficeCore" / L"updater.exe";
	if (_wcsicmp(helper.c_str(), expected_helper.c_str()) != 0 ||
		_wcsicmp(target.filename().c_str(), L"excel.exe") != 0 ||
		target.parent_path().lexically_normal() != staged.parent_path().lexically_normal() ||
		!std::filesystem::exists(staged)) {
		return ShowUpdateFailure(L"The update paths are invalid.");
	}

	HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, parent_pid);
	if (parent) {
		const DWORD wait_result = WaitForSingleObject(parent, 120000);
		CloseHandle(parent);
		if (wait_result != WAIT_OBJECT_0) {
			DeleteFileW(staged.c_str());
			return ShowUpdateFailure(L"The application did not close in time. The update was not installed.");
		}
	}
	else if (GetLastError() != ERROR_INVALID_PARAMETER) {
		DeleteFileW(staged.c_str());
		return ShowUpdateFailure(L"Could not wait for the application to close. The update was not installed.");
	}

	DWORD expected_type = 0;
	if (!GetBinaryTypeW(target.c_str(), &expected_type) ||
		!IsExpectedExecutable(staged, expected_type)) {
		DeleteFileW(staged.c_str());
		return ShowUpdateFailure(L"The downloaded file is not a compatible Windows executable.");
	}

	const std::filesystem::path backup =
		target.wstring() + L".backup-" + std::to_wstring(parent_pid) +
		L"-" + std::to_wstring(GetTickCount64());
	if (!ReplaceFileW(target.c_str(), staged.c_str(), backup.c_str(),
		REPLACEFILE_WRITE_THROUGH, nullptr, nullptr)) {
		DeleteFileW(staged.c_str());
		return ShowUpdateFailure(L"Windows could not replace excel.exe. The current version was left unchanged.");
	}

	std::wstring command_line = QuoteArgument(target.wstring());
	std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
	mutable_command.push_back(L'\0');
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	const std::wstring working_directory = target.parent_path().wstring();

	if (!CreateProcessW(target.c_str(), mutable_command.data(), nullptr, nullptr, FALSE, 0, nullptr,
		working_directory.c_str(), &startup, &process)) {
		const DWORD error = GetLastError();
		if (ReplaceFileW(target.c_str(), backup.c_str(), nullptr, REPLACEFILE_WRITE_THROUGH, nullptr, nullptr)) {
			DeleteFileW(staged.c_str());
			return ShowUpdateFailure(L"The updated application could not start. The previous version was restored.");
		}

		std::wstring message = L"The update was installed but could not be started or rolled back. "
			L"The backup is at:\n" + backup.wstring() + L"\nWindows error: " + std::to_wstring(error);
		MessageBoxW(nullptr, message.c_str(), L"Update Recovery Required", MB_OK | MB_ICONERROR);
		return 1;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	if (!DeleteFileW(backup.c_str())) {
		std::wstring message = L"The update was installed, but the old executable backup could not be removed:\n" +
			backup.wstring();
		MessageBoxW(nullptr, message.c_str(), L"Update Installed", MB_OK | MB_ICONWARNING);
	}
	return 0;
}
}

bool Updater::Init() {
	return GetInstance().InitImpl();
}

bool Updater::Update() {
	return GetInstance().ProcessImpl();
}

bool Updater::Process() {
	return GetInstance().ProcessImpl();
}

int Updater::HandleUpdateCommandLine() {
	int argument_count = 0;
	LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
	if (!arguments)
		return -1;

	if (argument_count < 2 || std::wstring_view(arguments[1]) != L"--apply-update") {
		LocalFree(arguments);
		return -1;
	}

	if (argument_count != 5) {
		LocalFree(arguments);
		return ShowUpdateFailure(L"The updater command is invalid.");
	}

	wchar_t* end = nullptr;
	errno = 0;
	const unsigned long parent_pid = std::wcstoul(arguments[2], &end, 10);
	if (errno == ERANGE || !end || *end != L'\0' || parent_pid == 0 || parent_pid > MAXDWORD) {
		LocalFree(arguments);
		return ShowUpdateFailure(L"The updater process identifier is invalid.");
	}

	const std::filesystem::path target(arguments[3]);
	const std::filesystem::path staged(arguments[4]);
	LocalFree(arguments);
	return ApplyUpdate(static_cast<DWORD>(parent_pid), target, staged);
}

Status Updater::GetStatus() {
	return GetInstance().status;
}

bool Updater::InitImpl() {
	status.current_version = current_version;
	status.latest_version.clear();
	status.update_check_succeeded = false;
	status.notice_title.clear();
	status.notice.clear();

	json response;
	const int http_status = HttpHelper::Get(release_url, response);
	if (http_status != 200) {
		LOGF(WARNING, "Could not check for updates (HTTP status {})", http_status);
		return false;
	}

	try {
		if (!response.is_object() || response.value("draft", true) || response.value("prerelease", true))
			throw std::runtime_error("Latest GitHub release is not a stable published release");

		const std::string tag = response.at("tag_name").get<std::string>();
		const std::string name = response.at("name").get<std::string>();
		std::array<std::uint64_t, 3> parsed_version{};
		if (!ParseVersion(tag, parsed_version))
			throw std::runtime_error("Release tag does not match vX.X.X");
		if (name != "Excel-Strike " + tag)
			throw std::runtime_error("Release name does not match its tag");

		release.tag = tag;
		status.latest_version = tag;
		if (response.contains("body") && response["body"].is_string())
			FormatReleaseNotes(response["body"].get_ref<const std::string&>(),
				status.notice_title, status.notice);
		if (CompareVersions(tag, current_version) != 0) {
			const std::string expected_prefix = std::string(RELEASE_DOWNLOAD_PREFIX) + tag + "/";
			bool found_executable = false;
			bool found_checksum = false;

			for (const auto& asset : response.at("assets")) {
				const std::string name = asset.at("name").get<std::string>();
				const std::string url = asset.at("browser_download_url").get<std::string>();
				if (name == "excel.exe") {
					if (url != expected_prefix + "excel.exe")
						throw std::runtime_error("Release executable URL is unexpected");
					release.executable_url = url;
					found_executable = true;
				}
				else if (name == "excel.exe.sha256") {
					if (url != expected_prefix + "excel.exe.sha256")
						throw std::runtime_error("Release checksum URL is unexpected");
					release.checksum_url = url;
					found_checksum = true;
				}
			}

			if (!found_executable || !found_checksum)
				throw std::runtime_error("Latest release is missing excel.exe or excel.exe.sha256");
		}
	}
	catch (const std::exception& error) {
		LOGF(WARNING, "Could not read the latest GitHub release: {}", error.what());
		return false;
	}

	isSetup = true;
	status.update_check_succeeded = true;
	LOGF(INFO, "Latest stable release is {}", release.tag);
	return true;
}

bool Updater::UpdateImpl() {
	if (!isSetup || CompareVersions(release.tag, current_version) == 0 ||
		release.executable_url.empty() || release.checksum_url.empty()) {
		LOGF(WARNING, "No different verified release is available");
		return false;
	}

	std::filesystem::path target;
	if (!GetExecutablePath(target)) {
		LOGF(WARNING, "Could not determine the application executable path");
		return false;
	}

	const std::wstring suffix = L".update-" + std::to_wstring(GetCurrentProcessId()) +
		L"-" + std::to_wstring(GetTickCount64()) + L".download";
	const std::filesystem::path staged = target.parent_path() /
		(target.filename().wstring() + suffix);

	const int download_status = HttpHelper::Download(release.executable_url, staged);
	if (download_status != 200) {
		LOGF(WARNING, "Update download failed with HTTP status {}", download_status);
		return false;
	}

	std::string checksum_contents;
	if (HttpHelper::GetText(release.checksum_url, checksum_contents) != 200) {
		LOGF(WARNING, "Could not download the update SHA-256 checksum");
		std::error_code error;
		std::filesystem::remove(staged, error);
		return false;
	}

	std::string expected_digest;
	if (!ParseChecksum(checksum_contents, expected_digest) || !VerifySha256(staged, expected_digest)) {
		LOGF(WARNING, "Downloaded update SHA-256 did not match the release checksum");
		std::error_code error;
		std::filesystem::remove(staged, error);
		return false;
	}

	DWORD current_binary_type = 0;
	DWORD downloaded_binary_type = 0;
	if (!GetBinaryTypeW(target.c_str(), &current_binary_type) ||
		!GetBinaryTypeW(staged.c_str(), &downloaded_binary_type) ||
		current_binary_type != downloaded_binary_type) {
		LOGF(WARNING, "Downloaded update is not a compatible Windows executable");
		std::error_code error;
		std::filesystem::remove(staged, error);
		return false;
	}

	const std::filesystem::path helper_directory = target.parent_path() / L"OfficeCore";
	const std::filesystem::path helper = helper_directory / L"updater.exe";
	std::error_code error;
	std::filesystem::create_directories(helper_directory, error);
	if (error || !std::filesystem::copy_file(
		target, helper, std::filesystem::copy_options::overwrite_existing, error)) {
		LOGF(WARNING, "Could not prepare OfficeCore/updater.exe: {}", error.message());
		std::filesystem::remove(staged, error);
		return false;
	}

	std::wstring command_line = QuoteArgument(helper.wstring()) + L" --apply-update " +
		std::to_wstring(GetCurrentProcessId()) + L" " + QuoteArgument(target.wstring()) +
		L" " + QuoteArgument(staged.wstring());
	std::vector<wchar_t> mutable_command(command_line.begin(), command_line.end());
	mutable_command.push_back(L'\0');
	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	const std::wstring working_directory = target.parent_path().wstring();

	if (!CreateProcessW(helper.c_str(), mutable_command.data(), nullptr, nullptr, FALSE, 0, nullptr,
		working_directory.c_str(), &startup, &process)) {
		LOGF(WARNING, "Could not start OfficeCore/updater.exe: Windows error {}", GetLastError());
		std::filesystem::remove(staged, error);
		return false;
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	LOGF(INFO, "Update {} downloaded and verified; closing for installation", release.tag);
	return true;
}

bool Updater::ProcessImpl() {
	if (!isSetup)
		return true;

	const int version_comparison = CompareVersions(release.tag, current_version);
	if (version_comparison == 0)
		return true;

	std::wstring message = L"Excel-Strike version ";
	message.append(release.tag.begin(), release.tag.end());
	message += version_comparison < 0
		? L" is the latest published version, which is older than your installed version. Downgrade now?"
		: L" is available. Download and install it now?";
	const int result = MessageBoxW(nullptr, message.c_str(),
		version_comparison < 0 ? L"Downgrade Available" : L"Update Available",
		MB_ICONINFORMATION | MB_YESNO);

	if (result != IDYES)
		return true;

	if (UpdateImpl())
		return false;

	MessageBoxW(nullptr, L"The update could not be installed. The current version will continue running.",
		L"Update Failed", MB_OK | MB_ICONWARNING);
	return true;
}
