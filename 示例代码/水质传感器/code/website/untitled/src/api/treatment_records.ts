import axios from 'axios';
const API_BASE = import.meta.env.VITE_API_BASE;
// 治理记录相关接口
export const getTreatmentRecords = async () => axios.get(`${API_BASE}/treatment_records`);
export const getTreatmentRecord = (id: number) => axios.get(`${API_BASE}/treatment_records/${id}`);
export const createTreatmentRecord = (data: any) => axios.post(`${API_BASE}/treatment_records`, data);
export const updateTreatmentRecord = (id: number, data: any) => axios.put(`${API_BASE}/treatment_records/${id}`, data);
export const deleteTreatmentRecord = (id: number) => axios.delete(`${API_BASE}/treatment_records/${id}`);

