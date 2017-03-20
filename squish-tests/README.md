fdupes -r . -d -N # delete all md5-sum identical images first, it's pretty quick
indimagedupes -t 99.5% --program=`which rm` . # 99.5% confidence interval seems to be what's needed to get allow 2 characters difference
