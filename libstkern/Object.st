nil subclass: Object [
    tester: someArgument [
        | localVariable |
        localVariable <- 100.
        someArgument <- 200.
        ^ [ localVariable doSomethingWithBlock: [ [ localVariable. someArgument <- 42. localVariable ] ] ]
    ]
]
