import { env, cwd } from 'node:process'
import { copyFileSync, existsSync } from 'node:fs'
import { resolve } from 'node:path'

const {
  ENV_CMAKE_PROJECT_NAME,
  ENV_CMAKE_TARGET_FILE_DIR,
  ENV_CMAKE_BUILD_CONFIG,
} = env
const allEnvPreset = [
  ENV_CMAKE_PROJECT_NAME,
  ENV_CMAKE_TARGET_FILE_DIR,
  ENV_CMAKE_BUILD_CONFIG,
].every(Boolean)
console.log(
  'environment variables:',
  {
    ENV_CMAKE_PROJECT_NAME,
    ENV_CMAKE_TARGET_FILE_DIR,
    ENV_CMAKE_BUILD_CONFIG,
    allEnvPreset,
  },
  '\n'
)

if (!allEnvPreset) {
  console.warn('Missing environment variables.\nSkipping c++ asset copy.')
} else {
  const projectName = ENV_CMAKE_PROJECT_NAME
  const targetDir = resolve(ENV_CMAKE_TARGET_FILE_DIR)
  const destDir = resolve(cwd(), 'public')
  for (const ext of ['data', 'html', 'js', 'wasm']) {
    const name = `${projectName}.${ext}`
    const pad = '    '.substring(ext.length)
    const [src, dest] = [targetDir, destDir].map(dir => resolve(dir, name))
    if (!existsSync(src)) continue
    copyFileSync(src, dest)
    console.log(`Copied ${src}${pad} -> ${dest}${pad}`)
  }
}
