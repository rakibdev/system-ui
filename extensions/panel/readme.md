```
notify-send -p "Rakib" "Hi" | { read id; sleep 2; notify-send -r "$id" "Rakib" "Hi (replaced)"; }
```
