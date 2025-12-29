add_rules("mode.debug", "mode.release")
package("cppjieba")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/yanyiwu/cppjieba")
    
    -- 🚨 关键修正：开启 submodules = true，否则 deps/limonp 是空的
    set_urls("https://github.com/yanyiwu/cppjieba.git", {submodules = true})
    
    on_install(function (package)
        -- 1. 复制 cppjieba 头文件
        os.cp("include/cppjieba", package:installdir("include"))
        
        -- 2. 复制 limonp 头文件
        -- limonp 的真实结构在 deps/limonp/include/limonp
        -- 我们需要把它复制到安装目录的 include/limonp 下
        if os.exists("deps/limonp/include/limonp") then
            os.cp("deps/limonp/include/limonp", package:installdir("include"))
        else
            -- 备用方案：如果目录结构不同，尝试直接复制 deps/limonp
            os.cp("deps/limonp", package:installdir("include"))
        end
    end)
package_end()

add_requires("cppjieba")

target("my_jieba_demo")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("cppjieba")
    set_languages("c++17")
    
    -- 设置运行目录
    set_rundir("$(projectdir)")

    -- 2. 自动下载 dict 资源 (保持不变)
    on_load(function (target)
        local dict_dir = path.join(os.projectdir(), "dict")
        if not os.exists(dict_dir) then
            print("⚠️  检测到缺少字典文件，正在自动从 GitHub 下载...")
            import("net.http")
            import("utils.archive")
            local url = "https://github.com/yanyiwu/cppjieba/archive/refs/heads/master.zip"
            local archive_file = "cppjieba_master.zip"
            local temp_dir = "temp_jieba_extract"
            http.download(url, archive_file)
            print("📦 正在解压资源...")
            archive.extract(archive_file, temp_dir)
            local source_dict = path.join(temp_dir, "cppjieba-master", "dict")
            os.mv(source_dict, dict_dir)
            os.rm(archive_file)
            os.rm(temp_dir)
            print("✅ 字典配置完成！")
        end
    end)
