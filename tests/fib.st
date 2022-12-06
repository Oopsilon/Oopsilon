nil subclass: SmallInteger [
    fib [
      ^ self > 1
        ifTrue: [ (self - 1) fib + (self - 2) fib ]
        ifFalse: [ self ]
    ]
]
