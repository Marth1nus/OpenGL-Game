var Module = {
  canvas: (() => {
    const canvas = document.querySelector('#canvas')
    if (!(canvas instanceof HTMLCanvasElement)) return
    let resizeTimeout
    const onResize = () => {
      clearTimeout(resizeTimeout)
      resizeTimeout = setTimeout(() => {
        Module?.setCanvasSize?.call(
          Module,
          canvas.clientWidth,
          canvas.clientHeight
        )
      }, 1000 / 60)
    }
    new ResizeObserver(onResize).observe(canvas)
    return canvas
  })(),
  print: (() => {
    let line = 0
    document.getElementById('output').value = ''
    return (...args) => {
      const text = args.join(' ')
      console.log(text)
      const element = document.querySelector('#output')
      if (!(element instanceof HTMLTextAreaElement)) return
      element.value += `${String(++line).padStart(4, ' ')}> ${text}\n`
      element.scrollTop = element.scrollHeight // focus on bottom
    }
  })(),
  onRuntimeInitialized() {
    const { canvas } = Module
    if (!canvas) throw new Error('No Canvas')
    if (!Module?.setCanvasSize) throw new Error('No Canvas Resize')
    setTimeout(() => {
      Module.setCanvasSize(canvas.clientWidth, canvas.clientHeight)
    }, 1)
  },
}
