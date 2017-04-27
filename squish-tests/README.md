fdupes -r . -d -N # delete all md5-sum identical images first, it's pretty quick
indimagedupes -t 99.5% --program=`which rm` . # 99.5% confidence interval seems to be what's needed to get allow 2 characters difference


# run stress tests on a loop
while :; do ./squishrunner --testsuite ~/Programs/Mudlet/Github-Mudlet/squish-tests/suite_Mudlet/ --testcase tst_open_custom_exit_lines_dialog; sleep 1; done