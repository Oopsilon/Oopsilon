nil subclass: Object [
    tester: someArgument [
        | counter |
        ^ 1 to: 100 do: [ :i | [ counter <- counter + i ] value ]
    ]
]
