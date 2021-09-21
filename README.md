# writed

writed is a tiny daemon that displays messages received via the traditional `write`/`wall` mechanism as desktop notifications via `libnotify`.

## building

You'll need `libnotify`, `pkg-config`, and a C compiler.

```
gcc -std=c89 -lutil `pkg-config --cflags --libs libnotify` main.c -o writed
```

writed **must** be setgid `utmp`. `write` and `wall` only send messages to `pts` devices registered in `/var/run/utmp`, and without setgid `utmp`, writed will silently fail to receive messages.

## usage

Just run `writed` while you have a desktop notification daemon running.
