/*
 * SPDX-License-Identifier: MIT License
 *
 * Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * Author(s):   Daniel Oliveira <daniel.oliveira@amd.com>
 *
 * Description: filesystem_ops.hpp
 *
 */

#if !defined(AMD_WORK_BENCH_HTTP_OPS_HPP)
#define AMD_WORK_BENCH_HTTP_OPS_HPP

#include <work_bench.hpp>
#include <awb/filesystem_ops.hpp>
#include <awb/logger.hpp>
#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <curl/curl.h>

#include <atomic>
#include <bitset>
#include <cstddef>
#include <filesystem>
// #include <format>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace amd_work_bench::http
{

static constexpr auto kHTTP_SUCCESS_CODE = u32_t(200);
static constexpr auto kHTTP_REQUEST_TIMEOUT_MS = u32_t(1000);
static constexpr auto kHTTP_HEADER_CACHE_CONTROL_CONTENT_TYPE = std::string_view{"Cache-Control: no-cache"};


using CurlPtr_t = void*;
using FSPath_t = std::filesystem::path;
using DataStream_t = std::vector<u8_t>;


class HTTPRequestResultBase_t
{
    public:
        HTTPRequestResultBase_t() = default;
        explicit HTTPRequestResultBase_t(u32_t http_status_code) : m_http_status_code(http_status_code), m_is_valid_request(true)
        {
        }

        [[nodiscard]] auto get_status_code() const noexcept -> u32_t
        {
            return m_http_status_code;
        }

        [[nodiscard]] auto is_valid_request() const noexcept -> bool
        {
            return m_is_valid_request;
        }

        [[nodiscard]] auto is_request_successful() const noexcept -> bool
        {
            return (m_is_valid_request && (m_http_status_code == kHTTP_SUCCESS_CODE));
        }


    private:
        u32_t m_http_status_code{0};
        bool m_is_valid_request{false};
};


class HTTPRequest_t
{
    public:
        HTTPRequest_t() = delete;
        HTTPRequest_t(std::string method, std::string url);    // TODO: std::call_once /  std::experimental::scope_exit

        HTTPRequest_t(const HTTPRequest_t&) = delete;
        HTTPRequest_t& operator=(const HTTPRequest_t&) = delete;

        HTTPRequest_t(HTTPRequest_t&& other) noexcept;
        HTTPRequest_t& operator=(HTTPRequest_t&& other) noexcept;
        ~HTTPRequest_t();

        template<typename Tp>
        class HTTPResult_t : public HTTPRequestResultBase_t
        {
            public:
                HTTPResult_t() = default;
                explicit HTTPResult_t(u32_t http_status_code, Tp request_data)
                    : HTTPRequestResultBase_t(http_status_code), m_request_data(std::move(request_data))
                {
                }

                [[nodiscard]] auto get_data() const noexcept -> const Tp&
                {
                    return m_request_data;
                }


            private:
                Tp m_request_data{};
        };

        template<typename Tp = std::string>
        std::future<HTTPResult_t<Tp>> file_download(const FSPath_t& file_path);
        std::future<HTTPResult_t<DataStream_t>> file_download();

        template<typename Tp = std::string>
        std::future<HTTPResult_t<Tp>> file_upload(const FSPath_t& file_path, const std::string& mime_type = "amdfile.amd");

        template<typename Tp = std::string>
        std::future<HTTPResult_t<Tp>> file_upload(DataStream_t request_data,
                                                  const std::string& mime_type = "amdfile.amd",
                                                  const FSPath_t& file_path = "amdfile.bin");

        template<typename Tp = std::string>
        std::future<HTTPResult_t<Tp>> run();


        static auto set_proxy_state(bool is_enabled) -> void;
        static auto set_prox_url(std::string proxy_url) -> void;
        auto set_timeout(u32_t timeout) -> void;
        auto set_method(std::string method) -> void;
        auto set_url(std::string url) -> void;
        auto set_header(std::string key, std::string value) -> void;
        auto set_body(std::string body) -> void;
        auto get_progress() const -> float;
        auto cancel_request() -> void;

        static auto url_encode(const std::string& data_sequence) -> std::string;
        static auto url_decode(const std::string& data_sequence) -> std::string;


    protected:
        auto set_default_request_configuration() -> void;

        template<typename Tp>
        HTTPResult_t<Tp> run_impl(DataStream_t& request_data);

        // void* memcpy( void* dest, const void* src, std::size_t count );
        static auto write_to_data_stream(void* src_buff, size_t buff_size, size_t num_mem_blocks, void* tgt_buff) -> size_t;
        static auto write_to_file(void* src_buff, size_t buff_size, size_t num_mem_blocks, void* tgt_buff) -> size_t;
        static auto progress_cb(void* src_buff,
                                curl_off_t total_download,
                                curl_off_t currently_downloaded,
                                curl_off_t total_upload,
                                curl_off_t currently_uploaded) -> u32_t;


    private:
        bool m_is_valid_request{false};
        std::mutex m_transmission;
        std::string m_method{};
        std::string m_url{};
        std::string m_body{};
        std::map<std::string, std::string> m_headers{};
        std::atomic<float> m_progress{0.0f};
        std::atomic<bool> m_was_canceled{false};
        std::promise<DataStream_t> m_promise{};
        u32_t m_http_status_code{0};
        u32_t m_timeout{kHTTP_REQUEST_TIMEOUT_MS};
        CurlPtr_t m_curl_handle{nullptr};

        static auto validate_proxy_errors() -> void;
};


/*
 *  TODO: Think about Cookie Management
 */
/*
class HTTPCookieManagement_t
{
    public:
        HTTPCookieManagement_t() = default;
        ~HTTPCookieManagement_t() = default;

        auto set_cookie(std::string key, std::string value) -> void;
        auto get_cookie(std::string key) -> std::string;
        auto remove_cookie(std::string key) -> void;
        auto clear_cookies() -> void;

    private:
        std::map<std::string, std::string> m_cookies{};
};
*/


template<typename Tp>
HTTPRequest_t::HTTPResult_t<Tp> HTTPRequest_t::run_impl(DataStream_t& request_data)
{
    /*
     *  Note:   set options for a curl easy handle
     */
    curl_easy_setopt(m_curl_handle, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(m_curl_handle, CURLOPT_CUSTOMREQUEST, m_method.c_str());

    set_default_request_configuration();
    if (!m_body.empty()) {
        curl_easy_setopt(m_curl_handle, CURLOPT_POSTFIELDS, m_body.c_str());
    }

    /*
     *  Note:   linked-list structure
     */
    curl_slist* headers(nullptr);
    headers = curl_slist_append(headers, std::string(kHTTP_HEADER_CACHE_CONTROL_CONTENT_TYPE).c_str());
    /*
     *  TODO:   std::experimental::scope_exit
     *  on_exit { (curl_slist_free_all, headers); };
     */
    for (const auto& [key, value] : m_headers) {
        headers = curl_slist_append(headers, amd_fmt::format("{}: {}", key, value).c_str());
    }
    curl_easy_setopt(m_curl_handle, CURLOPT_HTTPHEADER, headers);

    {
        std::lock_guard<std::mutex> lock(m_transmission);
        if (auto curl_op_result = curl_easy_perform(m_curl_handle); curl_op_result != CURLE_OK) {
            char* url_ptr = nullptr;
            curl_easy_getinfo(m_curl_handle, CURLINFO_EFFECTIVE_URL, &url_ptr);
            wb_logger::loginfo(LogLevel::LOGGER_ERROR,
                               "Error handing HTTP Request: '{} {}'. Failed with error: {}: {}",
                               m_method,
                               url_ptr,
                               static_cast<u32_t>(curl_op_result),
                               curl_easy_strerror(curl_op_result));
            validate_proxy_errors();
            return {};
        }
    }

    auto curl_status_code = ilong_t(0);
    curl_easy_getinfo(m_curl_handle, CURLINFO_RESPONSE_CODE, &curl_status_code);
    /*
     *  Note:   Check if request_data is enough, or if
     *          {request_data.begin(), request_data.end()} is needed
     */
    return HTTPResult_t<Tp>(m_http_status_code, request_data);
}


template<typename Tp>
std::future<HTTPRequest_t::HTTPResult_t<Tp>> HTTPRequest_t::file_download(const FSPath_t& file_path)
{
    return std::async(std::launch::async, [this, file_path]() -> HTTPResult_t<Tp> {
        DataStream_t response_data{};

        /*
        TODO:   File file(file_path, File::Mode::CREATE);
        */
        wb_fs_io::FileOps_t download_file(file_path, FileOpsMode_t::CREATE);
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_to_file);
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &download_file);
        return run_impl<Tp>(response_data);
    });
}


template<typename Tp>
std::future<HTTPRequest_t::HTTPResult_t<Tp>> HTTPRequest_t::file_upload(const FSPath_t& file_path, const std::string& mime_type)
{
    return std::async(std::launch::async, [this, file_path, mime_type]() -> HTTPResult_t<Tp> {
        auto file_name = file_path.filename().string();
        curl_mime* mime_name = curl_mime_init(m_curl_handle);
        curl_mimepart* mime_part = curl_mime_addpart(mime_name);

        /*
        TODO:   File file(file_path, File::Mode::READ);
                Work the CBs below.
        */
        wb_fs_io::FileOps_t upload_file(file_path, FileOpsMode_t::READ);
        auto read_cb = []() -> size_t {
            return 0;
        };
        auto seek_cb = []() -> size_t {
            return 0;
        };
        auto free_cb = []() -> size_t {
            return 0;
        };

        /*
         *  Note:   set a callback-based data source for a mime part body
         */
        //  TODO: Implement this properly, when time comes.
        // curl_mime_data_cb(mime_part, upload_file.get_size(), read_cb, seek_cb, free_cb, upload_file.get_handle());
        /*
         *  Note:   set a mime part remote filename
         */
        //  TODO: Implement this properly, when time comes.
        // curl_mime_filename(mime_part, file_name.c_str());
        /*
         *  Note:   set a mime part name
         */
        //  TODO: Implement this properly, when time comes.
        // curl_mime_name(mime_part, mime_name.c_str());
        curl_easy_setopt(m_curl_handle, CURLOPT_MIMEPOST, mime_name);

        DataStream_t response_data{};
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_to_data_stream);
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &response_data);
        return run_impl<Tp>(response_data);
    });
}


template<typename Tp>
std::future<HTTPRequest_t::HTTPResult_t<Tp>> HTTPRequest_t::file_upload(DataStream_t data_stream,
                                                                        const std::string& mime_type,
                                                                        const FSPath_t& file_path)
{
    return std::async(std::launch::async,
                      [this, data_stream = std::move(data_stream), mime_type, file_path]() -> HTTPResult_t<Tp> {
                          auto file_name = file_path.filename().string();
                          curl_mime* mime_name = curl_mime_init(m_curl_handle);
                          curl_mimepart* mime_part = curl_mime_addpart(mime_name);

                          curl_mime_data(mime_part, reinterpret_cast<const char*>(data_stream.data()), data_stream.size());
                          curl_mime_filename(mime_part, file_name.c_str());
                          curl_mime_name(mime_part, mime_type.c_str());
                          curl_easy_setopt(m_curl_handle, CURLOPT_MIMEPOST, mime_name);

                          DataStream_t response_data{};
                          curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_to_data_stream);
                          curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &response_data);
                          return run_impl<Tp>(response_data);
                      });
}


template<typename Tp>
std::future<HTTPRequest_t::HTTPResult_t<Tp>> HTTPRequest_t::run()
{
    return std::async(std::launch::async, [this]() -> HTTPResult_t<Tp> {
        DataStream_t response_data{};
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_to_data_stream);
        curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, &response_data);
        return run_impl<Tp>(response_data);
    });
}


}    // namespace amd_work_bench::http


// namespace alias
namespace wb_http = amd_work_bench::http;

#endif    //-- AMD_WORK_BENCH_HTTP_OPS_HPP
