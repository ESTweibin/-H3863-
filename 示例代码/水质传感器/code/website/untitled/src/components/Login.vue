<template>
  <div class="login-container">
    <el-card class="login-card">
      <template #header>
        <div class="card-header">
          <span>系统登录</span>
        </div>
      </template>
      <el-form :model="loginForm" @submit.prevent="handleLogin">
        <el-form-item label="用户名">
          <el-input v-model="loginForm.username" placeholder="请输入用户名"></el-input>
        </el-form-item>
        <el-form-item label="密码">
          <el-input v-model="loginForm.password" type="password" placeholder="请输入密码" show-password></el-input>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" native-type="submit" style="width: 100%;">登录</el-button>
        </el-form-item>
      </el-form>
      <el-alert v-if="loginError" :title="loginError" type="error" show-icon :closable="false"></el-alert>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { reactive, ref } from 'vue';
import { useRouter } from 'vue-router';
import { ElForm, ElFormItem, ElInput, ElButton, ElCard, ElAlert } from 'element-plus';
import  { login } from '../api/users';

const router = useRouter();
const loginForm = reactive({
  username: '',
  password: ''
});
const loginError = ref('');

const handleLogin = async () => {
  loginError.value = '';
  // 简单的模拟登录逻辑
  let data  = await login({
    "email": loginForm.username,
    "password": loginForm.password

  });
  if (data.data.success == 0) {
    localStorage.setItem('isAuthenticated', 'true');
    router.push('/'); // 登录成功后跳转到仪表盘
  } else {
    loginError.value = '用户名或密码错误';
    localStorage.removeItem('isAuthenticated');
  }
};
</script>

<style scoped>
.login-container {
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100vh;
  background-color: #0A192F; /* 深蓝色背景 */
}

.login-card {
  width: 400px;
  background-color: #172A45; /* 卡片背景色 */
  border: 1px solid #173A5E;
}

.card-header span {
  color: #64FFDA; /* 亮眼的青色标题 */
  font-size: 1.5em;
}

:deep(.el-form-item__label) {
  color: #E0E0E0; /* 标签文字颜色 */
}

:deep(.el-input__wrapper) {
  background-color: #0A192F !important; /* 输入框背景 */
  box-shadow: 0 0 0 1px #173A5E inset !important;
}

:deep(.el-input__inner) {
  color: #E0E0E0 !important; /* 输入文字颜色 */
}

.el-button--primary {
  background-color: #64FFDA;
  border-color: #64FFDA;
  color: #0A192F;
}

.el-button--primary:hover {
  background-color: #52D8C9;
  border-color: #52D8C9;
}

.el-alert {
  margin-top: 15px;
}
</style>

