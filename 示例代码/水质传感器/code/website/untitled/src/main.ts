import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import './style.css' // 保留原有或全局样式
import App from './App.vue'
import router from './router' // 导入路由

const app = createApp(App)

app.use(ElementPlus)
app.use(router) // 使用路由
app.mount('#app')
