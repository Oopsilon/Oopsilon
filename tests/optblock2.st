nil subclass: Object [
    tester: someArgument [
        | localVariable |
        ^ 0 ifTrue: [ localVariable <- 59 ] ifFalse: [  | optBlockLocal | [ ^optBlockLocal] ] 
    ]
]
