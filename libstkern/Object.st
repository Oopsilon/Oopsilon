nil subclass: Object [
    tester [
        | localVariable |
        ^ localVariable doSomethingWithBlock: [ localVariable <- 42. localVariable ]
    ]
]
