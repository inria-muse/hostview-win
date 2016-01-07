# Compiling curl in Visual Studio 2015 Community Ed.

Hostview uses libcurl for data uploads. The library is statically linked to the project 
and a prebuild version is included with the source code. However, if you need to re-build 
the libcurl for some reason (e.g. switching visual studio versions), here's how:

## Obtain Prerequisites
	
    $ git clone https://github.com/peters/curl-for-windows.git
    $ git submodule update --init --recursive	

## Generate Project Files

 	$ python configure.py

If configure fails, see [issue 7](https://github.com/peters/curl-for-windows/issues/7]).

## Fix openssl (for VS2015)

diff --git a/libssh2 b/libssh2
--- a/libssh2
+++ b/libssh2
@@ -1 +1 @@
-Subproject commit f1cfa55b6064ba18fc0005713ed790da579361b5
+Subproject commit f1cfa55b6064ba18fc0005713ed790da579361b5-dirty
diff --git a/openssl/openssl/e_os.h b/openssl/openssl/e_os.h
index 79c1392..b6f7ca2 100644
--- a/openssl/openssl/e_os.h
+++ b/openssl/openssl/e_os.h
@@ -307,7 +307,7 @@ static unsigned int _strlen31(const char *str)
 #      undef isxdigit
 #    endif
 #    if defined(_MSC_VER) && !defined(_DLL) && defined(stdin)
-#      if _MSC_VER>=1300
+#      if _MSC_VER>=1300 && _MSC_VER <= 1800
 #        undef stdin
 #        undef stdout
 #        undef stderr

## Build

Open  curl.sln in visual studio and do Build -> Batch ... -> Select All -> Build
