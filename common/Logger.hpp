//
// Logger.hpp
//

#pragma once
#include <iostream>
#include "unicode.hpp"

#ifndef _WIN32
#define OutputDebugString(str) std::cout << str
#endif

class Logger {
        // Singleton
    private:
        Logger() {}
        Logger(const Logger& logger) {}
        virtual ~Logger() {}

    public:
        static void Info(const tstring& format) {
            getInstance().Log(_T("INFO: "), format);
        }

        template<class T1>
        static void Info(const tstring& format, const T1& t1) {
            getInstance().Log(_T("INFO: "), format, t1);
        }

        template<class T1, class T2>
        static void Info(const tstring& format, const T1& t1, const T2& t2) {
            getInstance().Log(_T("INFO: "), format, t1, t2);
        }

        template<class T1, class T2, class T3>
        static void Info(const tstring& format, const T1& t1, const T2& t2, const T3& t3) {
            getInstance().Log(_T("INFO: "), format, t1, t2, t3);
        }

        template<class T1, class T2, class T3, class T4>
        static void Info(const tstring& format, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
            getInstance().Log(_T("INFO: "), format, t1, t2, t3, t4);
        }


        static void Error(const tstring& format) {
            getInstance().Log(_T("ERROR: "), format);
        }

        template<class T1>
        static void Error(const tstring& format, const T1& t1) {
            getInstance().Log(_T("ERROR: "), format, t1);
        }

        template<class T1, class T2>
        static void Error(const tstring& format, const T1& t1, const T2& t2) {
            getInstance().Log(_T("ERROR: "), format, t1, t2);
        }

        template<class T1, class T2, class T3>
        static void Error(const tstring& format, const T1& t1, const T2& t2, const T3& t3) {
            getInstance().Log(_T("ERROR: "), format, t1, t2, t3);
        }

        template<class T1, class T2, class T3, class T4>
        static void Error(const tstring& format, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
            getInstance().Log(_T("ERROR: "), format, t1, t2, t3, t4);
        }


        static void Debug(const tstring& format) {
            getInstance().Log(_T("DEBUG: "), format);
        }

        template<class T1>
        static void Debug(const tstring& format, const T1& t1) {
            getInstance().Log(_T("DEBUG: "), format, t1);
        }

        template<class T1, class T2>
        static void Debug(const tstring& format, const T1& t1, const T2& t2) {
            getInstance().Log(_T("DEBUG: "), format, t1, t2);
        }

        template<class T1, class T2, class T3>
        static void Debug(const tstring& format, const T1& t1, const T2& t2, const T3& t3) {
            getInstance().Log(_T("DEBUG: "), format, t1, t2, t3);
        }

        template<class T1, class T2, class T3, class T4>
        static void Debug(const tstring& format, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
            getInstance().Log(_T("DEBUG: "), format, t1, t2, t3, t4);
        }


    private:
        typedef boost::basic_format<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>> tformat;

        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        void Log(const tstring& prefix, const tstring& format) {
            auto out = prefix + format + _T("\r\n");
            OutputDebugString(prefix.c_str());
            OutputDebugString(format.c_str());
            OutputDebugString(_T("\r\n"));
        }

        template<class T1>
        void Log(const tstring& prefix, const tstring& format, const T1& t1) {
            OutputDebugString(prefix.c_str());
            OutputDebugString((tformat(format) % t1).str().c_str());
            OutputDebugString(_T("\r\n"));
        }

        template<class T1, class T2>
        void Log(const tstring& prefix, const tstring& format, const T1& t1, const T2& t2) {
            OutputDebugString(prefix.c_str());
            OutputDebugString((tformat(format) % t1 % t2).str().c_str());
            OutputDebugString(_T("\r\n"));
        }

        template<class T1, class T2, class T3>
        void Log(const tstring& prefix, const tstring& format, const T1& t1, const T2& t2, const T3& t3) {
            OutputDebugString(prefix.c_str());
            OutputDebugString((tformat(format) % t1 % t2 % t3).str().c_str());
            OutputDebugString(_T("\r\n"));
        }

        template<class T1, class T2, class T3, class T4>
        void Log(const tstring& prefix, const tstring& format, const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
            OutputDebugString(prefix.c_str());
            OutputDebugString((tformat(format) % t1 % t2 % t3 % t4).str().c_str());
            OutputDebugString(_T("\r\n"));
        }
};