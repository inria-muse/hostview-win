# Compiling curl in Visual Studio 2015 Community Ed.

## Obtain Prerequisites 
	
    $ git clone https://github.com/apietila/curl-for-windows.git
    $ git submodule update --init --recursive	

## Generate Project Files

 	$ python configure.py

If configure fails, see [issue 7](https://github.com/peters/curl-for-windows/issues/7]).

## Fix openssl

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

Open  curl.sln in VC2015 and do Build -> Batch ... -> Select All -> Build/