/*
 * MIT License
 *
 * Copyright (c) 2024, Advanced Micro Devices, Inc. All rights reserved.
 *
 *  Developed by:
 *
 *                  AMD ML Software Engineering
 *
 *                  Advanced Micro Devices, Inc.
 *
 *                  www.amd.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimers in
 *    the documentation and/or other materials provided with the distribution.
 *  - Neither the names of Advanced Micro Devices, Inc,
 *    nor the names of its contributors may be used to endorse or promote
 *    products derived from this Software without specific prior written
 *    permission.
 *
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
 * Author(s):   Daniel Oliveira <daniel.oliveira@amd.com>
 *
 *
 * Description: dynlib_mgmt.hpp
 *
 */

#include <cpp_std_support/include/cppstd_hooks.hpp>

#include <cstdint>
#include <exception>
#include <filesystem>
// #include <format>
#include <functional>
#include <iostream>
#include <thread>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>


#if !defined(AMD_DYNLIB_MGMT_HPP)
#define AMD_DYNLIB_MGMT_HPP

// clang-format off
#if !defined(AMD_API_VISIBILITY_SIMBOL) && defined (__GNUC__) && (__GNUC__ >= 4)
    #define AMD_API_VISIBILITY_SIMBOL
    #define AMD_DYN_LIB_VISIBILITY_PREFIX extern "C" [[gnu::visibility("default")]]
    #define AMD_DYN_LIB_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
    #define AMD_DYN_LIB_SYMBOL_HIDDEN __attribute__((__visibility__("hidden")))
    #define AMD_DYN_LIB_SYMBOL_UNUSED __attribute__((unused))
#endif  // AMD_API_VISIBILITY_SIMBOL
// clang-format on


namespace amd_shared_library_mgmt
{

class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryIsLoadedException_t : public std::runtime_error
{
    public:
        SharedLibraryIsLoadedException_t(const std::string& message) : std::runtime_error(message)
        {
        }

        ~SharedLibraryIsLoadedException_t() noexcept
        {
        }


    private:
        //


    protected:
};


class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryLoadException_t : public std::runtime_error
{
    public:
        SharedLibraryLoadException_t(const std::string& message) : std::runtime_error(message)
        {
        }

        ~SharedLibraryLoadException_t() noexcept
        {
        }


    private:
        //


    protected:
};


class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryNotFoundException_t : public std::runtime_error
{
    public:
        SharedLibraryNotFoundException_t(const std::string& message) : std::runtime_error(message)
        {
        }

        ~SharedLibraryNotFoundException_t() noexcept
        {
        }


    private:
        //


    protected:
};


class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryNullPtrException_t : public std::logic_error
{
    public:
        SharedLibraryNullPtrException_t(const std::string& message) : std::logic_error(message)
        {
        }

        ~SharedLibraryNullPtrException_t() noexcept
        {
        }


    private:
        //


    protected:
};


class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryInvalidAccessException_t : public std::logic_error
{
    public:
        SharedLibraryInvalidAccessException_t(const std::string& message) : std::logic_error(message)
        {
        }

        ~SharedLibraryInvalidAccessException_t() noexcept
        {
        }


    private:
        //


    protected:
};


/*
 *  Singleton helper for singleton instances on heap.
 *  Proper deletion is handled by it when application that created them is terminated.
 */
template<typename SingletonTp>
class SingletonHolder_t
{
    public:
        SingletonHolder_t() : m_instance_ptr(0)
        {
        }

        ~SingletonHolder_t()
        {
            if (m_instance_ptr) {
                delete m_instance_ptr;
            }
        }

        auto get_instance() -> SingletonTp*
        {
            std::lock_guard<std::mutex> lock(m_instance_mutex);
            if (!m_instance_ptr) {
                m_instance_ptr = new SingletonTp();
            }

            return m_instance_ptr;
        }

        auto reset_instance() -> void
        {
            std::lock_guard<std::mutex> lock(m_instance_mutex);
            if (m_instance_ptr) {
                delete m_instance_ptr;
                m_instance_ptr = nullptr;
            }
        }


    private:
        SingletonTp* m_instance_ptr{nullptr};
        std::mutex m_instance_mutex{};

    protected:
        //
};


/*
 *  C++ class information are stored here.
 */
template<typename BaseTp>
class AbstractMetaInfo_t
{
    public:
        AbstractMetaInfo_t(const char* class_name) : m_class_name(class_name)
        {
        }

        virtual ~AbstractMetaInfo_t()
        {
            for (auto delete_ptr : m_delete_list) {
                delete delete_ptr;
            }
        }

        auto class_name() const -> const char*
        {
            return m_class_name;
        }

        virtual auto create() const -> BaseTp* = 0;
        virtual auto instance() const -> BaseTp& = 0;
        virtual auto is_create_allowed() const -> bool = 0;
        virtual auto destroy(BaseTp* instance) const -> void
        {
            auto instance_itr = m_delete_list.find(instance);
            if (instance_itr != m_delete_list.end()) {
                m_delete_list.erase(instance);
                delete instance;
            }
        }

        auto auto_delete(BaseTp* instance) const -> BaseTp*
        {
            // So we dodge the singletons
            if (is_create_allowed()) {
                if (instance) {
                    m_delete_list.insert(instance);
                } else {
                    throw SharedLibraryNullPtrException_t("Error: Instance is nullptr.");
                }
            } else {
                const auto exception_message_helper = amd_fmt::format("Error: Auto delete not allowed: {}", class_name());
                throw SharedLibraryInvalidAccessException_t(exception_message_helper);
            }

            return instance;
        }

        virtual auto is_auto_delete_allowed(BaseTp* instance) const -> bool
        {
            return (m_delete_list.contains(instance));
        }


    private:
        AbstractMetaInfo_t();
        AbstractMetaInfo_t(const AbstractMetaInfo_t&);
        AbstractMetaInfo_t& operator=(const AbstractMetaInfo_t&);

        using ObjectSet_t = std::set<BaseTp*>;
        const char* m_class_name{nullptr};
        mutable ObjectSet_t m_delete_list{};


    protected:
        //
};

/*
 *  This is the base class for all classes that will be used to build the manifest.
 *  It can be used as an object factory
 */
template<typename DerivedTp, typename BaseTp>
class MetaInfo_t : public AbstractMetaInfo_t<BaseTp>
{
    public:
        MetaInfo_t(const char* class_name) : AbstractMetaInfo_t<BaseTp>(class_name)
        {
        }

        ~MetaInfo_t() = default;

        auto create() const -> BaseTp* override
        {
            //  Make sure the DerivedTp class has a default constructor
            return new DerivedTp();
        }

        /*
         *  Not a singleton instance
         */
        auto instance() const -> BaseTp& override
        {
            const auto exception_message_helper =
                amd_fmt::format("Error: Use create() to get an instance: {} ", this->class_name());
            throw SharedLibraryInvalidAccessException_t(exception_message_helper);
        }

        auto is_create_allowed() const -> bool override
        {
            return true;
        }

    private:
        //

    protected:
        //
};


template<typename DerivedTp, typename BaseTp>
class MetaInfoSingleton_t : public AbstractMetaInfo_t<BaseTp>
{
    public:
        MetaInfoSingleton_t(const char* class_name) : AbstractMetaInfo_t<BaseTp>(class_name)
        {
        }

        ~MetaInfoSingleton_t() = default;

        /*
         *  Creation of a singleton instance is not allowed
         */
        auto create() const -> BaseTp* override
        {
            const auto exception_message_helper =
                amd_fmt::format("Error: Use instance() to get an instance: {} ", this->class_name());
            throw SharedLibraryInvalidAccessException_t(exception_message_helper);
        }

        auto instance() const -> BaseTp& override
        {
            return *m_instance.get_instance();
        }

        auto is_create_allowed() const -> bool override
        {
            return false;
        }

        auto is_auto_delete_allowed([[maybe_unused]] BaseTp* instance) const -> bool override
        {
            return true;
        }

    private:
        mutable SingletonHolder_t<DerivedTp> m_instance{};


    protected:
        //
};


/*
 *  This is the base class for all classes that will be used to build the manifest.
 */
class AMD_DYN_LIB_SYMBOL_VISIBLE ManifestBase_t
{
    public:
        ManifestBase_t() = default;
        virtual ~ManifestBase_t() = default;
        virtual auto class_identity_name() -> const char* = 0;

    private:
        //

    protected:
};

/*
 *  This is where we maintain a list of all the classes contained in a shared library class
 */
template<typename BaseTp>
class Manifest_t : public ManifestBase_t
{
    public:
        using MetaInfo_t = AbstractMetaInfo_t<BaseTp>;
        using MetaInfoMap_t = std::map<std::string, const MetaInfo_t*>;

        Manifest_t() = default;
        virtual ~Manifest_t()
        {
            cleanup();
        };

        auto class_identity_name() -> const char* override
        {
            return typeid(*this).name();
        }

        auto insert(const MetaInfo_t* meta_info) -> bool
        {
            //  dependent-name required here
            return m_meta_info_map.insert(typename MetaInfoMap_t::value_type(meta_info->class_name(), meta_info)).second;
        }

        auto cleanup() -> void
        {
            for (auto& meta_info : m_meta_info_map) {
                delete meta_info.second;
            }
            m_meta_info_map.clear();
        }

        auto size() const -> size_t
        {
            return m_meta_info_map.size();
        }

        auto is_empty() const -> bool
        {
            return m_meta_info_map.empty();
        }


        class Iterator_t
        {
            public:
                Iterator_t(const MetaInfoMap_t::const_iterator& itr) : m_itr(itr)
                {
                }

                Iterator_t(const Iterator_t& itr) : m_itr(itr.m_itr)
                {
                }

                ~Iterator_t() = default;

                auto operator=(const Iterator_t& itr) -> Iterator_t&
                {
                    m_itr = itr.m_itr;
                    return *this;
                }

                auto operator++() -> Iterator_t&
                {
                    ++m_itr;
                    return *this;
                }

                auto operator++(int) -> Iterator_t
                {
                    Iterator_t temp_itr(m_itr);
                    ++m_itr;
                    return temp_itr;
                }

                inline auto operator==(const Iterator_t& itr) const -> bool
                {
                    return (m_itr == itr.m_itr);
                }

                inline auto operator!=(const Iterator_t& itr) const -> bool
                {
                    return (m_itr != itr.m_itr);
                }

                inline auto operator*() -> const MetaInfo_t*
                {
                    return m_itr->second;
                }

                inline auto operator->() -> const MetaInfo_t*
                {
                    return m_itr->second;
                }


            private:
                MetaInfoMap_t::const_iterator m_itr;


            protected:
                //
        };


        auto begin() const -> Iterator_t
        {
            return Iterator_t(m_meta_info_map.begin());
        }

        auto end() const -> Iterator_t
        {
            return Iterator_t(m_meta_info_map.end());
        }

        auto find(const std::string& class_name) const -> Iterator_t
        {
            return Iterator_t(m_meta_info_map.find(class_name));
        }


    private:
        MetaInfoMap_t m_meta_info_map{};
        //

    protected:
        //
};

/*
 *  All implementation details are hidden from the user.
 */
class SharedLibraryDetails_t
{
    public:
        //


    private:
        void* m_library_handle{nullptr};
        std::string m_library_path{};
        static std::mutex m_library_mutex;


    protected:
        enum class DetailFlags_t : int32_t
        {
            NONE_SHARED_LIB_DETAILS = 0x00,
            GLOBAL_SHARED_LIB_DETAILS = 0x01,
            LOCAL_SHARED_LIB_DETAILS = 0x02,
        };

        SharedLibraryDetails_t();
        ~SharedLibraryDetails_t();

        auto load_details(const std::string& library_path, DetailFlags_t flags) -> void;
        auto unload_details() -> void;
        auto is_loaded_details() const -> bool;
        auto get_symbol_details(const std::string& symbol_name) -> void*;
        auto get_path_details() const -> const std::string&;
        static auto get_suffix_details() -> std::string;
        static auto set_search_path_details(const std::string& search_path) -> bool;
};


/*
 *  This class represents the shared library (object; myplugin.so, myplugin.amdplugin, etc).
 */
class AMD_DYN_LIB_SYMBOL_VISIBLE SharedLibraryMgmt_t : private SharedLibraryDetails_t
{
    public:
        enum class SharedLibraryFlags_t : int32_t
        {
            /*
             *  Note:   Some plats use don't dlopen(), we should use: RTLD_GLOBAL (default)
             *          and ignored on plats that do not use dlopen(). GLOBAL_SHARED_LIB
             *
             *          Some other plats using dlopen, we should use: RTLD_LOCAL
             *
             *          If this flag is specified, RTTI (including dynamic_cast and throw) will
             *          not work for types defined in the shared library with GCC/Clang.
             *
             *          See: http://gcc.gnu.org/faq.html#dso for more information.
             *
             *          How do they work? dlopen() and dlsym():
             *          -   When we dlopen() a library, all the functions defined by that library
             *              become available in your virtual address space due to the fact that
             *              dlopen calls mmap() multiple times).
             *          -   dlsym() doesn't add (or load) any additional code, that is already there.
             *          -   It (dlsym()) provides is the ability to get the address of something in
             *              that shared library from its name, using some dynamic symbol table in that
             *              ELF plugin.
             *
             */
            NONE_SHARED_LIB = 0x00,
            GLOBAL_SHARED_LIB = 0x01,
            LOCAL_SHARED_LIB = 0x02,
        };


        SharedLibraryMgmt_t();
        SharedLibraryMgmt_t(const std::string& library_path);
        SharedLibraryMgmt_t(const std::string& library_path, SharedLibraryFlags_t library_flags);
        virtual ~SharedLibraryMgmt_t();

        auto load(const std::string& library_path) -> void;
        auto load(const std::string& library_path, SharedLibraryFlags_t library_flags) -> void;
        auto unload() -> void;
        auto is_loaded() const -> bool;
        auto has_symbol(const std::string& symbol_name) -> bool;
        auto get_symbol(const std::string& symbol_name) -> void*;
        auto get_path() const -> const std::string&;
        static auto get_suffix() -> std::string;
        static auto set_search_path(const std::string& search_path) -> bool;


    private:
        SharedLibraryMgmt_t(const SharedLibraryMgmt_t&);
        SharedLibraryMgmt_t& operator=(const SharedLibraryMgmt_t&);


    protected:
};


/*
 *  This class is used to load C++ classes from shared libraries during runtime.
 *  It has to instantiate the base class for the classes being loaded.
 *  So, for plugin, for example: 'class Plugin_t : public PluginIface_t'
 *  The base class is 'PluginIface_t'.
 */
template<typename BaseTp>
class ClassWorker_t
{
    public:
        using MetaInfoTp_t = AbstractMetaInfo_t<BaseTp>;
        using ManifestTp_t = Manifest_t<BaseTp>;
        using BuildManifestFunc_t = bool (*)(ManifestBase_t*);
        using InitLibFunc_t = void (*)();
        using DeInitLibFunc_t = void (*)();

        /*
         *  Note:   These symbols are used to call the API functions in the shared library.
         *          Search for: 'bool AMD_DYN_LIB_SYMBOL_VISIBLE shared_library_mgmt_build_manifest().'
         */
        const std::string kBUILD_MANIFEST_API_SYMBOL{"shared_library_mgmt_build_manifest"};
        const std::string kINITIALIZE_LIBRARY_API_SYMBOL{"shared_library_mgmt_initialize_library"};
        const std::string kDEINITIALIZE_LIBRARY_API_SYMBOL{"shared_library_mgmt_deinitialize_library"};

        struct LibraryDetails_t
        {
            public:
                std::shared_ptr<SharedLibraryMgmt_t> m_library_mgmt{nullptr};
                // SharedLibraryMgmt_t* m_library_mgmt{nullptr};
                std::string m_library_path{};
                SharedLibraryMgmt_t::SharedLibraryFlags_t m_library_flags{
                    SharedLibraryMgmt_t::SharedLibraryFlags_t::NONE_SHARED_LIB};
                std::shared_ptr<ManifestTp_t> m_manifest{nullptr};
                // const Manifest_t* m_manifest{nullptr};
                int32_t m_ref_count{0};

            private:
                //

            protected:
        };
        using LibraryManifestMap_t = std::map<std::string, LibraryDetails_t>;

        class Iterator
        {
            public:
                using LibraryManifestPair_t = std::pair<const std::string, const ManifestTp_t*>;

                Iterator(const LibraryManifestMap_t::const_iterator& itr) : m_itr(itr)
                {
                }

                Iterator(const Iterator& itr) : m_itr(itr.m_itr)
                {
                }

                ~Iterator() = default;

                auto operator=(const Iterator& itr) -> Iterator&
                {
                    m_itr = itr.m_itr;
                    return *this;
                }

                auto operator++() -> Iterator&
                {
                    ++m_itr;
                    return *this;
                }

                auto operator++(int) -> Iterator
                {
                    Iterator temp_itr(m_itr);
                    ++m_itr;
                    return temp_itr;
                }

                inline auto operator==(const Iterator& itr) const -> bool
                {
                    return (m_itr == itr.m_itr);
                }

                inline auto operator!=(const Iterator& itr) const -> bool
                {
                    return (m_itr != itr.m_itr);
                }

                inline auto operator*() const -> const LibraryManifestPair_t*
                {
                    m_library_manifest_pair.first = m_itr->first;
                    m_library_manifest_pair.second = m_itr->second.m_manifest;
                    return &m_library_manifest_pair;
                }

                inline auto operator->() const -> const LibraryManifestPair_t*
                {
                    m_library_manifest_pair.first = m_itr->first;
                    m_library_manifest_pair.second = m_itr->second.m_manifest;
                    return &m_library_manifest_pair;
                }


            private:
                LibraryManifestMap_t::const_iterator m_itr;
                mutable LibraryManifestPair_t m_library_manifest_pair;

            protected:
        };


        ClassWorker_t() = default;
        virtual ~ClassWorker_t() = default;

        // TODO: Delete this block if all works fine with smart-pointers.
        /*
        {
            for (auto& library_details : m_library_manifest_map) {
                if (library_details.second.m_library_mgmt) {
                    delete library_details.second.m_library_mgmt;
                }
                if (library_details.second.m_manifest) {
                    delete library_details.second.m_manifest;
                }
            }
        }
        */


        /*
         *  This function loads (if not already loaded, otherwise it doesn't do anything) the shared library from a
         *  given path by using the manifest symbol. If there is no manifest symbol or the library cannot be loaded,
         *  an exception is thrown.
         *
         *  If 'shared_library_mgmt_initialize_library()' is exported, it will be executed.
         *  The number of calls to load_library() and unload_library() must match.
         *
         */
        auto load_library(const std::string& library_path, const std::string& manifest_symbol) -> void
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            auto library_itr = m_library_manifest_map.find(library_path);
            if (library_itr == m_library_manifest_map.end()) {
                auto library_details = LibraryDetails_t{};
                library_details.m_ref_count = 1;

                try {
                    library_details.m_library_mgmt = std::make_shared<SharedLibraryMgmt_t>(library_path);
                    // new SharedLibraryMgmt_t(library_path);
                    library_details.m_manifest = std::make_shared<ManifestTp_t>();
                    // new Manifest_t();

                    //  If the init library call is available, call it
                    if (library_details.m_library_mgmt->has_symbol(kINITIALIZE_LIBRARY_API_SYMBOL)) {
                        auto init_library_func = reinterpret_cast<InitLibFunc_t>(
                            library_details.m_library_mgmt->get_symbol(kINITIALIZE_LIBRARY_API_SYMBOL));
                        init_library_func();
                    }

                    //  If the library is not loaded, we load it
                    auto symbol_build_manifest(kBUILD_MANIFEST_API_SYMBOL);
                    symbol_build_manifest.append(manifest_symbol);
                    if (library_details.m_library_mgmt->has_symbol(symbol_build_manifest)) {
                        auto build_manifest_func = reinterpret_cast<BuildManifestFunc_t>(
                            library_details.m_library_mgmt->get_symbol(symbol_build_manifest));
                        if (build_manifest_func(const_cast<ManifestTp_t*>(library_details.m_manifest.get()))) {
                            // if (build_manifest_func(const_cast<Manifest_t*>(library_details.m_manifest))) {
                            m_library_manifest_map.emplace(std::make_pair(library_path, library_details));
                        } else {
                            const auto exception_message_helper =
                                amd_fmt::format("Error: Failed to build manifest class: {} -> {}", library_path, manifest_symbol);
                            throw SharedLibraryLoadException_t(exception_message_helper);
                        }
                    } else {
                        const auto exception_message_helper =
                            amd_fmt::format("Error: Manifest build class not found: {} -> {}", library_path, manifest_symbol);
                        throw exception_message_helper;
                    }
                } catch (...) {
                    //  Note:   No need to delete the pointers (smart) here, the destructor will take care of it.
                    throw;
                }

            } else {
                ++library_itr->second.m_ref_count;
            }
        }

        auto load_library(const std::string& library_path) -> void
        {
            load_library(library_path, std::string());
        }

        auto load_library(const std::filesystem::path& library_path) -> void
        {
            load_library(library_path.string());
        }


        /*
         *  This function unloads (if loaded) the shared library from a given path.
         *  Note:   Be very careful when using this function, as it objects in this library could be still in use,
         *          and referenced by other objects. That's why we try to keep 'm_ref_count' updated, but not
         *          everything goes through that path in the code.
         *          If that is the case, then we likely will have a crash.
         *
         *  given path by using the manifest symbol. If there is no manifest symbol or the library cannot be loaded,
         *  an exception is thrown.
         *
         *  If 'shared_library_mgmt_deinitialize_library()' is exported, it will be executed (before unloading the
         *  library).
         *
         *  The number of calls to load_library() and unload_library() must match.
         *
         */
        auto unload_library(const std::string& library_path) -> void
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            auto library_itr = m_library_manifest_map.find(library_path);
            if (library_itr == m_library_manifest_map.end()) {
                const auto exception_message_helper = amd_fmt::format("Error: Library not found: {}", library_path);
                throw SharedLibraryNotFoundException_t(exception_message_helper);
            } else {
                if (--library_itr->second.m_ref_count == 0) {
                    //  If the deinit library call is available, call it
                    if (library_itr->second.m_library_mgmt->has_symbol(kDEINITIALIZE_LIBRARY_API_SYMBOL)) {
                        auto deinit_library_func = reinterpret_cast<DeInitLibFunc_t>(
                            library_itr->second.m_library_mgmt->get_symbol(kDEINITIALIZE_LIBRARY_API_SYMBOL));
                        deinit_library_func();
                    }

                    library_itr->second.m_library_mgmt->unload();
                    m_library_manifest_map.erase(library_itr);
                }
            }
        }


        /*
         *  This function returns a pointer to the MetaInfo_t object for the given class name or nullptr if the
         *  class was not found.
         */
        auto find_class(const std::string& class_name) const -> const MetaInfoTp_t*
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            for (const auto& library_details : m_library_manifest_map) {
                if (auto manifest_itr = library_details.second.m_manifest->find(class_name);
                    manifest_itr != library_details.second.m_manifest->end()) {
                    return *manifest_itr;
                }
            }

            return nullptr;
        }

        /*
         *  This function returns a reference to the MetaInfo_t object for the given class name or exception if
         *  the class was not found.
         */
        auto find_class_meta_info(const std::string& class_name) const -> const MetaInfoTp_t&
        {
            auto meta_info = find_class(class_name);
            if (meta_info) {
                return *meta_info;
            } else {
                const auto exception_message_helper = amd_fmt::format("Error: Class not found: {}", class_name);
                throw SharedLibraryNotFoundException_t(exception_message_helper);
            }
        }


        auto create_instance(const std::string& class_name) const -> BaseTp*
        {
            return find_class_meta_info(class_name).create();
        }


        auto get_instance(const std::string& class_name) const -> BaseTp&
        {
            return find_class_meta_info(class_name).instance();
        }


        auto is_create_allowed(const std::string& class_name) const -> bool
        {
            return find_class_meta_info(class_name).is_create_allowed();
        }


        auto destroy_instance(const std::string& class_name, BaseTp* instance) const -> void
        {
            find_class_meta_info(class_name).destroy(instance);
        }


        auto is_auto_delete_allowed(const std::string& class_name, BaseTp* instance) const -> bool
        {
            return find_class_meta_info(class_name).is_auto_delete_allowed(instance);
        }


        auto find_build_manifest(const std::string& library_path) const -> const ManifestTp_t*
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            if (const auto library_itr = m_library_manifest_map.find(library_path); library_itr != m_library_manifest_map.end()) {
                return library_itr->second.m_manifest.get();
            }

            return nullptr;
        }


        auto find_build_manifest_meta_info(const std::string& library_path) const -> const MetaInfoTp_t&
        {
            if (const auto manifest = find_build_manifest(library_path); manifest) {
                return *manifest;
            } else {
                const auto exception_message_helper = amd_fmt::format("Error: Library not found: {}", library_path);
                throw SharedLibraryNotFoundException_t(exception_message_helper);
            }
        }


        auto is_library_loaded(const std::string& library_path) const -> bool
        {
            return (find_build_manifest(library_path) != nullptr);
        }


        auto begin() const -> Iterator
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            return Iterator(m_library_manifest_map.begin());
        }

        auto end() const -> Iterator
        {
            std::scoped_lock lock(m_library_manifest_mutex);
            return Iterator(m_library_manifest_map.end());
        }


    private:
        LibraryManifestMap_t m_library_manifest_map{};
        mutable std::mutex m_library_manifest_mutex{};


    protected:
};


}    // namespace amd_shared_library_mgmt


/*
 *  The C entry points for every library
 */
extern "C"
{
    /*
     *  Search for the API name "_symbol_name" in the ClassWorker_t instance.
     */
    bool AMD_DYN_LIB_SYMBOL_VISIBLE shared_library_mgmt_build_manifest(amd_shared_library_mgmt::ManifestBase_t* manifest);
    void AMD_DYN_LIB_SYMBOL_VISIBLE shared_library_mgmt_initialize_library();
    void AMD_DYN_LIB_SYMBOL_VISIBLE shared_library_mgmt_deinitialize_library();
}

/*
 *  We will replace the macros with constexpr and inline function when we are able to fully support:
 *  '#code and ##'
 *  Note:   Be aware that inside macros and extern "C" statements we are not using 'auto -> return trailing type'.
 *          Better no push our luck when using them and exporting symbols, due to some compilers not supporting it
 *          and some others issues we had to debug.
 */
//  Joins the args together, even if one arg is a macro itself. The important point here is that
//  the macro expansion of the args happens in AMD_DYN_LIB_JOIN_ARGS_DO_IT (not in AMD_DYN_LIB_JOIN_ARGS_DO_IT_IMPL)
#define AMD_DYN_LIB_JOIN_ARGS(X, Y) AMD_DYN_LIB_JOIN_ARGS_DO_IT(X, Y)
#define AMD_DYN_LIB_JOIN_ARGS_DO_IT(X, Y) AMD_DYN_LIB_JOIN_ARGS_DO_IT_IMPL(X, Y)
#define AMD_DYN_LIB_JOIN_ARGS_DO_IT_IMPL(X, Y) X##Y

// clang-format off
#define AMD_SHARED_LIBRARY_MANIFEST_BUILD(manifest)                                                        \
    extern "C"                                                                                             \
    {                                                                                                      \
        bool AMD_DYN_LIB_SYMBOL_VISIBLE AMD_DYN_LIB_JOIN_ARGS(shared_library_mgmt_build_manifest, manifest)\
        (amd_shared_library_mgmt::ManifestBase_t* manifest);                                               \
    }



// These implement the shared_library_mgmt_build_manifest() 'automagically'
#define AMD_SHARED_LIBRARY_MGMT_MANIFEST_BEGIN(base_class) \
    AMD_SHARED_LIBRARY_MGMT_MANIFEST_BEGIN_IMPL(shared_library_mgmt_build_manifest, base_class)

#define AMD_SHARED_LIBRARY_MGMT_MANIFEST_BEGIN_IMPL(function_name, base_class)      \
    bool function_name(amd_shared_library_mgmt::ManifestBase_t* manifest_base_ptr)  \
    {                                                                               \
        using BaseType_t = base_class;                                              \
        using ManifestType_t = amd_shared_library_mgmt::Manifest_t<BaseType_t>;     \
        const std::string require_type(typeid(ManifestType_t).name());              \
        const std::string current_type(manifest_base_ptr->class_identity_name());   \
        if (current_type == require_type) {                                         \
            amd_shared_library_mgmt::Manifest_t<BaseType_t>* manifest = static_cast<ManifestType_t*>(manifest_base_ptr);

#define AMD_SHARED_LIBRARY_MGMT_MANIFEST_END    \
            return true;                        \
        }                                       \
                                                \
        return false;                           \
    }

#define AMD_SHARED_LIBRARY_MGMT_EXPORT_SINGLETON(class_name)    \
    manifest->insert(new amd_shared_library_mgmt::MetaInfoSingleton_t<class_name, BaseType_t>(#class_name));

#define AMD_SHARED_LIBRARY_MGMT_EXPORT_INTERFACE(class_name, interface_name)    \
    manifest->insert(new amd_shared_library_mgmt::MetaInfo_t<class_name, BaseType_t>(interface_name));

#define AMD_SHARED_LIBRARY_MGMT_EXPORT_CLASS(class_name)    \
    manifest->insert(new amd_shared_library_mgmt::MetaInfo_t<class_name, BaseType_t>(#class_name));


// clang-format on


#endif    // AMD_DYNLIB_MGMT_HPP
