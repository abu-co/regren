# regren

A minimalistic, cross-platform tool for batch renaming files with regular expressions. 

## Examples

Windows:
```batch
regren sandbox 123(.+)321(\.?.*) "$$file_$1$2" -i
```
Bash:
```bash
regren sandbox "123(.+)321(\\.?.*)" "\$\$file_\$1\$2" -i
```

Suppose the executable is called from a directory with the following content:
```
.
└── sandbox
    ├── 1233321
    ├── 123abc321.a
    └── abc123cba321.a
```
The command above will yield the following changes to `./sandbox/`: 

| Before           | After            |
|------------------|------------------|
| `1233321`        | `$file_3`        |
| `123abc321.a`    | `$file_abc.a`    |
| `abc123cba321.a` | `abc$file_cba.a` |

In order to ignore partial matches like `"abc123cba321.a"`, use `^` and `$`.

Run `regren --help` for more information on the options available.

## Implementation

The application is written in C++ 17, using only the standard library and [Microsoft's implementation of C++ GSL](https://github.com/microsoft/GSL).  
Therefore, the regular expressions are processed using an instance of `std::regex` constructed with the `std::regex::ECMAScript` flag turned on. 
This means that the regex syntax has certain subtle differences from the one actually used in ECMAScript. 
See "[Modified ECMAScript regular expression grammar](https://en.cppreference.com/w/cpp/regex/ecmascript)" for more information.

