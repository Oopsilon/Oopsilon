nil subclass: Object [
    tester: someArgument [
        | localVariable |
        ^ [ :arga :argb | | blockLocal | 0 ifTrue: [ localVariable <- 59 ] ifFalse: [  | optBlockLocal | blockLocal + optBlockLocal + argb ] ] value
    ]
]
