import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 用户相关接口
export const getUsers = () => axios.get(`${API_BASE}/users`);
export const getUser = (id: number) => axios.get(`${API_BASE}/users/${id}`);
export const createUser = (data: any) => axios.post(`${API_BASE}/users`, data);
export const updateUser = (id: number, data: any) => axios.put(`${API_BASE}/users/${id}`, data);
export const deleteUser = (id: number) => axios.delete(`${API_BASE}/users/${id}`);
export const login = async (data: any) => axios.post(`${API_BASE}/user/login`,data);
