dnl ##
dnl ##
dnl ##
define(AC_USE_FUNCTION,[dnl
AC_CHECK_FUNCS($1)
if test ".$ac_func_$1" = .yes; then 
AC_DEFINE(USE_$2)
fi
])dnl
dnl ##
dnl ##
dnl ##
define(AC_CHECK_DEFINE,[dnl
AC_CACHE_CHECK(for $1 in $2, ac_cv_define_$1,
AC_EGREP_CPP([YES_IS_DEFINED], [
#include <$2>
#ifdef $1
YES_IS_DEFINED
#endif
], ac_cv_define_$1=yes; AC_DEFINE(HAVE_$1), ac_cv_define_$1=no)
)])dnl
dnl ##
dnl ##
dnl ##
define(AC_IFALLYES,[dnl
ac_rc=yes
for ac_spec in $1; do
    ac_type=`echo "$ac_spec" | sed -e 's/:.*$//'`
    ac_item=`echo "$ac_spec" | sed -e 's/^.*://'`
    case $ac_type in
        header )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_header_$ac_item"
            ;;
        file )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_file_$ac_item"
            ;;
        func )   ac_var="ac_cv_func_$ac_item"   ;;
        define ) ac_var="ac_cv_define_$ac_item" ;;
        custom ) ac_var="$ac_item" ;;
    esac
    eval "ac_val=\$$ac_var"
    if test ".$ac_val" != .yes; then
        ac_rc=no
        break
    fi
done
if test ".$ac_rc" = .yes; then
    :
    $2
else
    :
    $3
fi
])dnl
dnl ##
dnl ##
dnl ##
define(AC_BEGIN_DECISION,[dnl
ac_decision_item='$1'
ac_decision_msg='FAILED'
ac_decision=''
])dnl
define(AC_DECIDE,[dnl
ac_decision='$1'
ac_decision_msg='$2'
ac_decision_$1=yes
ac_decision_$1_msg='$2'
])dnl
define(AC_DECISION_OVERRIDE,[dnl
    ac_decision=''
    for ac_item in $1; do
         eval "ac_decision_this=\$ac_decision_${ac_item}"
         if test ".$ac_decision_this" = .yes; then
             ac_decision=$ac_item
             eval "ac_decision_msg=\$ac_decision_${ac_item}_msg"
         fi
    done
])dnl
define(AC_DECISION_FORCE,[dnl
ac_decision="$1"
eval "ac_decision_msg=\"\$ac_decision_${ac_decision}_msg\""
])dnl
define(AC_END_DECISION,[dnl
if test ".$ac_decision" = .; then
    echo "[$]0:Error: decision on $ac_decision_item failed" 1>&2
    exit 1
else
    if test ".$ac_decision_msg" = .; then
        ac_decision_msg="$ac_decision"
    fi
    AC_DEFINE_UNQUOTED(${ac_decision_item})
    AC_MSG_RESULT([decision on $ac_decision_item... $ac_decision_msg])
fi
])dnl

dnl ### AC_TRY_RUN had some problems actually using a programs return code,
dnl ### so I am re-working it here to be used in APR's configure script.
dnl MY_TRY_RUN(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE
dnl            [, ACTION-IF-CROSS-COMPILING]]])
AC_DEFUN(MY_TRY_RUN,
[if test "$cross_compiling" = yes; then
  ifelse([$4], ,
    [errprint(__file__:__line__: warning: [AC_TRY_RUN] called without default to allow cross compiling
)dnl
  AC_MSG_ERROR(can not run test program while cross compiling)],
  [$4])
else
  MY_TRY_RUN_NATIVE([$1], [$2], [$3])
fi
])

dnl Like AC_TRY_RUN but assumes a native-environment (non-cross) compiler.
dnl MY_TRY_RUN_NATIVE(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE]])
AC_DEFUN(MY_TRY_RUN_NATIVE,
[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
extern "C" void exit(int);
#endif
])dnl
[$1]
EOF
if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext} && (./conftest; exit) 2>/dev/null
then
dnl Don't remove the temporary files here, so they can be examined.
  ifelse([$2], , :, [$2])
else
ifelse([$3], , , [  $3 
  rm -fr conftest*
])dnl
fi
rm -fr conftest*])



