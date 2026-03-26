/// <reference types="vite/client" />

interface ImportMetaEnv {
    readonly VITE_API_BASE_URL: string;
    // 在这里添加更多自定义的环境变量
}

interface ImportMeta {
    readonly env: ImportMetaEnv;
}