## Build & Run

```bash
xmake f -m release && xmake -P .
./build/theme --color "#ff5722"
```

## Dependencies

This tool requires the following system libraries:

- **cairo**: For image surface handling
- **libwebp**: For WebP image support
- **libjpeg-turbo**: For JPEG/JPG image support

## Variables

Use in templates with `{variable}` format:

- `{foreground}` `{background}` `{card}` `{popover}` `{hover}`
- `{primary}` `{primaryForeground}` `{secondary}` `{secondaryForeground}`
- `{border}`

### Variants

- `{variable.hexDigits}` - Hex color without the `#` prefix
