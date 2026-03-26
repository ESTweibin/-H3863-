# -*- coding: utf-8 -*-
"""
SnailObservations表的ORM模型定义
"""
from models import db
from datetime import datetime

class SnailObservations(db.Model):
    __tablename__ = 'SnailObservations'
    observation_id = db.Column(db.Integer, primary_key=True)
    latitude = db.Column(db.Numeric(10, 8), nullable=False)
    longitude = db.Column(db.Numeric(11, 8), nullable=False)
    timestamp = db.Column(db.DateTime, nullable=False)
    apple_snail_count = db.Column(db.Integer, nullable=False, default=0)
    notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        """
        将观测记录对象转换为字典，便于序列化为JSON
        """
        return {
            'observation_id': self.observation_id,
            'latitude': float(self.latitude),
            'longitude': float(self.longitude),
            'timestamp': self.timestamp,
            'apple_snail_count': self.apple_snail_count,
            'notes': self.notes,
            'created_at': self.created_at,
            'updated_at': self.updated_at
        }

