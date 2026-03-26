import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 福寿螺观测相关接口
export const getSnailObservations = async () => axios.get(`${API_BASE}/snail_observations`);
export const getSnailObservation = async (id: number) => axios.get(`${API_BASE}/snail_observations/${id}`);
export const createSnailObservation = async (data: any) => axios.post(`${API_BASE}/snail_observations`, data);
export const updateSnailObservation = async (id: number, data: any) => axios.put(`${API_BASE}${id}`, data);
export const deleteSnailObservation = async (id: number) => axios.delete(`${API_BASE}/snail_observations/${id}`);

