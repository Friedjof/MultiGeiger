import { defineConfig } from 'vite';
import { readFileSync } from 'fs';
import { fileURLToPath } from 'url';
import { resolve, dirname } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));

// Read version from VERSION file
function readVersion() {
  try {
    const versionRaw = readFileSync(resolve(__dirname, '../VERSION'), 'utf-8').trim();
    const version = versionRaw.replace(/^\/+\s*/, '');
    return version || 'dev';
  } catch (e) {
    console.warn('VERSION file not found, using "dev"');
    return 'dev';
  }
}

export default defineConfig({
  root: '.',
  publicDir: 'public',
  appType: 'spa',
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    minify: 'esbuild',
    esbuild: {
      drop: ['console', 'debugger']
    },
    rollupOptions: {
      output: {
        manualChunks: undefined,
        entryFileNames: 'assets/[name].[hash].js',
        chunkFileNames: 'assets/[name].[hash].js',
        assetFileNames: 'assets/[name].[hash][extname]'
      }
    }
  },
  define: {
    __APP_VERSION__: JSON.stringify(readVersion())
  },
  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://192.168.4.1',
        changeOrigin: true
      }
    }
  }
});
