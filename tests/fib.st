nil subclass: SmallInteger [
    fib: someArgument [
      ^ self > 1
        ifTrue: [ (self - 1) fib + (self - 2) fib ]
        ifFalse: [ self ]
    ]
]
