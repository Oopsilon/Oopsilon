nil subclass: Object [
    tester: someArgument [
        | localVariable |
        ^ [ :arga :argb | | realBlockLocal | 0 ifTrue: [ localVariable <- 59 ] ifFalse: [  | optBlockLocal | realBlockLocal + optBlockLocal + argb ] ] value
    ]
]
