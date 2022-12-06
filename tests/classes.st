nil subclass: Object [
  | objectIvar |
    fib: someArgument [
      ^ someArgument
    ]
]

Object subclass: Derived [
  | derivedIvar |
    fib: someArgument [
      ^ someArgument + objectIvar + derivedIvar
    ]
]
