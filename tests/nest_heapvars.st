nil subclass: SmallInteger [
    makeheapvars: someArgument [
      ^ 0 doItWith: [ [ someArgument <- 42. ^someArgument ] ]
    ]
]
