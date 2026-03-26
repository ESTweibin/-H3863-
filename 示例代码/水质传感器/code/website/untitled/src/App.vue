<template>
  <router-view></router-view>
</template>

<script setup lang="ts">
// No longer need to import individual components here
</script>

<style scoped>
/* App.vue can now have minimal or global styles if needed,
   as component-specific styles are in Dashboard.vue */
#app {
  height: 100vh;
  width: 100vw;
  margin: 0;
  padding: 0;
}
</style>

