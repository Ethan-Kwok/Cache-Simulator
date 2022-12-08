all : OneLevelCache

OneLevelCache : OneLevelCache.c
	gcc -Wall -Werror -fsanitize=address -std=c11 OneLevelCache.c -o OneLevelCache -lm

TwoLevelCache : TwoLevelCache.c
	gcc -Wall -Werror -fsanitize=address -std=c11 TwoLevelCache.c -o TwoLevelCache -lm