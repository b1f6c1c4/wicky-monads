# wicky-monads

- Update `./foo.inc` if and only if `./codegen` emits a different content.

    ```bash
    wic -o ./foo.inc ./codegen
    ```

- Run `./test1`, report failure in `./rpt1.monad` instead of returning error:

    ```bash
    monad -o ./rpt1 -- ./test1
    ```

- Run `./test2` for a maximum of 1 minute, report timeout and/or failure in `./rpt2.monad`:

    ```bash
    monad -t 1:00 -o ./rpt2 -- ./test2
    ```

- Run `wc -l` on `./rpt1` and `./rpt2` only if both were OK, write everything to `./final.json`:

    ```bash
    monad -o M:./final.json ./rpt1 ./rpt2 -- wc -l
    ```

