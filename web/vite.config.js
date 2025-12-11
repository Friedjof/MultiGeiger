import { defineConfig } from 'vite';
import { readFileSync } from 'fs';
import { resolve } from 'path';

// Read version from VERSION file
function readVersion() {
  try {
    const version = readFileSync(resolve(__dirname, '../VERSION'), 'utf-8').trim();
    return version || 'dev';
  } catch (e) {
    console.warn('VERSION file not found, using "dev"');
    return 'dev';
  }
}

export default defineConfig({
  root: '.',
  publicDir: 'public',
  build: {
    outDir: 'dist',
    emptyOutDir: true,
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true,
        drop_debugger: true
      }
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
