import { createRouter, createWebHistory } from 'vue-router';
import type { RouteRecordRaw } from 'vue-router';
import Login from '../components/Login.vue';
import Dashboard from '../components/Dashboard.vue';

const routes: Array<RouteRecordRaw> = [
  {
    path: '/login',
    name: 'Login',
    component: Login,
    meta: { requiresGuest: true } // 用户已登录则重定向
  },
  {
    path: '/',
    name: 'Dashboard',
    component: Dashboard,
    meta: { requiresAuth: true } // 需要登录才能访问
  }
];

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes
});

router.beforeEach((to, from, next) => {
  const isAuthenticated = localStorage.getItem('isAuthenticated') === 'true';

  if (to.meta.requiresAuth && !isAuthenticated) {
    // 此路由需要身份验证，但用户未通过身份验证，则重定向到登录页面。
    next({ name: 'Login' });
  } else if (to.meta.requiresGuest && isAuthenticated) {
    // 如果用户已通过身份验证，并且尝试访问登录页面等仅限访客的路由，则重定向到仪表盘。
    next({ name: 'Dashboard' });
  } else {
    // 否则，一切正常，允许导航。
    next();
  }
});

export default router;

