//^ self > 1
//			ifTrue: [ (self - 1) fib + (self - 2) fib ]
//			ifFalse: [ self ]

function myfib(number) {
	if ( number > 1)
		return myfib(number - 1) + myfib(number - 2)
	else
		return number
}

for (var i = 0; i < 10; i++)
myfib(34)
