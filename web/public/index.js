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
          canvas.clientHeight,
        )
      }, 1000 / 60)
    }
    new ResizeObserver(onResize).observe(canvas)
    return canvas
  })(),
  print: (() => {
    const termDiv = document.getElementById('term')
    if (!(termDiv instanceof HTMLDivElement))
      throw new Error('Terminal div missing')
    const term = new Terminal({
      cols: 120,
      rows: 16,
      cursorBlink: false,
      theme: {
        background: '#111',
        foreground: '#ddd',
      },
      rendererType: 'canvas',
      disableStdin: true,
    })
    window.term = term
    term.open(termDiv)
    return (...args) => {
      term.write(args.join(' ') + '\r\n')
    }
  })(),
  printErr: console.error,
  onRuntimeInitialized() {
    const { canvas } = Module
    if (!canvas) throw new Error('No Canvas')
    if (!Module?.setCanvasSize) throw new Error('No Canvas Resize')
    setTimeout(() => {
      Module.setCanvasSize(canvas.clientWidth, canvas.clientHeight)
    }, 1)
  },
}
function addGameDemoButtons() {
  const buttons = document.getElementById('demo-state-buttons')
  if (!buttons) throw new Error('buttons div missing')
  for (const [i, name] of ['Boids', 'Game of file'].entries()) {
    const button = document.createElement('button')
    button.innerHTML = `<pre>Start Demo ${1 + i} ${`"${name}"`.padEnd(16)}</pre>`
    button.addEventListener('click', () => {
      const canvas = document.getElementById('canvas')
      if (!canvas) throw new Error('canvas div missing')
      canvas.dispatchEvent(new KeyboardEvent('keydown', { keyCode: 18 }))
      canvas.dispatchEvent(new KeyboardEvent('keydown', { keyCode: 49 + i }))
      canvas.dispatchEvent(new KeyboardEvent('keyup', { keyCode: 18 }))
      canvas.dispatchEvent(new KeyboardEvent('keyup', { keyCode: 49 + i }))
    })
    buttons.appendChild(button)
  }
}
addGameDemoButtons()
