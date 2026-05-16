```bash
xmake f -m release && xmake -P .
./build/theme --color "#ff0062"
```

## Dependencies

- **cairo**: For image surface handling
- **libwebp**: For WebP image support
- **libjpeg-turbo**: For JPEG/JPG image support

## Template Variables

- `{foreground}` `{mutedForeground}` `{background}`
- `{card}` `{popover}` `{hover}`
- `{primary}` `{primaryForeground}` `{secondary}` `{secondaryForeground}`
- `{border}`

### Color Formats

- `{variable.hexDigits}` - Hex color without the `#` prefix
