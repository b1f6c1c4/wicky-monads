# wicky-monads

> Pain-reliever for build systems.

## Basic Use Cases

- Update `./foo.inc` if and only if `./codegen` emits a different content.

    ```bash
    wic -o ./foo.inc -- ./codegen
    ```
  
    - Using `wic` together with `ninja`'s `restat` can significantly reduce
      the amount of re-bulids when any sort of code generation is present.

- Run `./test1`, report failure in `./rpt1.monad` instead of returning error:

    ```bash
    monad -o ./rpt1 -- ./test1
    ```

    - Using `monad(1)` is better than `make --keep-going` because the execution
      details are saved into `<output>.monad`.
    - Furthermore, `monad(1)` also saves the amount of time and memory use
      during execution into `<output>.monad`.

- Run `./test2` for a maximum of 1 minute, report timeout and/or failure in `./rpt2.monad`:

    ```bash
    monad -t 1:00 -o ./rpt2 -- ./test2
    ```
  
    - `monad(1)` is better than `timeout(1)` because the latter doesn't have
      a chance to save the execution detail (e.g. memory used before killing.)

- Run `wc -l` on `./rpt1` and/or `./rpt2` (whichever succeeded,) write everything to `./final.json`:

    ```bash
    monad -o M:./final.json ./rpt1 ./rpt2 -- wc -l
    ```
  
    - `monad(1)` here will be very helpful if you want to gather all the
      results together, whether succeeded or failed, into a big file for
      data visualization.

## Usage

### `wic(1)`: Write-If-Changed

```bash
wic -o <output> -- <executable> <arg>...
```

### `monad(1)`: Monoid in the Category of Endofunctors

```bash
monad [-t <limit>] -o <output> <input>... -- <executable> <arg>...
```
