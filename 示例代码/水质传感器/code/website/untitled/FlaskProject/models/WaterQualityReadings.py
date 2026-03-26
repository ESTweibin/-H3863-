# -*- coding: utf-8 -*-
"""
WaterQualityReadings表的ORM模型定义
"""
from models import db
from datetime import datetime

class WaterQualityReadings(db.Model):
    __tablename__ = 'WaterQualityReadings'
    reading_id = db.Column(db.Integer, primary_key=True)
    latitude = db.Column(db.Numeric(10, 8), nullable=False)
    longitude = db.Column(db.Numeric(11, 8), nullable=False)
    timestamp = db.Column(db.DateTime, nullable=False)
    temperature_celsius = db.Column(db.Numeric(5, 2))
    ph_value = db.Column(db.Numeric(4, 2))
    dissolved_oxygen_mgl = db.Column(db.Numeric(5, 2))
    turbidity_ntu = db.Column(db.Numeric(6, 2))
    conductivity_us_cm = db.Column(db.Numeric(7, 2))
    source = db.Column(db.String(50), default='manual')
    notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        """
        将水质读数对象转换为字典，便于序列化为JSON
        """
        return {
            'reading_id': self.reading_id,
            'latitude': float(self.latitude),
            'longitude': float(self.longitude),
            'timestamp': self.timestamp,
            'temperature_celsius': float(self.temperature_celsius) if self.temperature_celsius is not None else None,
            'ph_value': float(self.ph_value) if self.ph_value is not None else None,
            'dissolved_oxygen_mgl': float(self.dissolved_oxygen_mgl) if self.dissolved_oxygen_mgl is not None else None,
            'turbidity_ntu': float(self.turbidity_ntu) if self.turbidity_ntu is not None else None,
            'conductivity_us_cm': float(self.conductivity_us_cm) if self.conductivity_us_cm is not None else None,
            'source': self.source,
            'notes': self.notes,
            'created_at': self.created_at,
            'updated_at': self.updated_at
        }

