#^ self > 1
#			ifTrue: [ (self - 1) fib + (self - 2) fib ]
#			ifFalse: [ self ]

def myfib(number):
	if  number > 1:
		return myfib(number - 1) + myfib(number - 2)
	else:
		return number

myfib(34)
