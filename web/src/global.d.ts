export {}

declare global {
  interface Window {
    // Provided by Emscripten bundle loaded via <script src="/uniscript.js">
    createUniscriptModule?: () => Promise<any>
  }
}

