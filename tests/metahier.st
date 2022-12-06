"if we ever move to a traditional Smalltalk-80 metaclass hierarchy (rather than
 the current simplified ObjC-style hierarchy) this will be for testing the
 bootstrap compiler."

nil subclass: Object [
  | objectIvar |
    fib: someArgument [
      ^ someArgument
    ]
]

Object subclass: Behavior [
  | methodDictionary |
    aBehaviorMethod [
        ^ methodDictionary.
    ]
]

Behavior subclass: ClassDescription [
  | instanceVariables |
    aClassDescriptionMethod [
        ^ instanceVariables + methodDictionary.
    ]
]

ClassDescription subclass: Class [
  | classVar |
    aClassMethod [
        ^ instanceVariables + classVar.
    ]
]

ClassDescription subclass: Metaclass [
  | metaclassVar |
    aMetaclassMethod [
        ^ metaclassVar + instanceVariables.
    ]
]
