# -*- coding: utf-8 -*-
"""
Alerts表的ORM模型定义
"""
from models import db
from datetime import datetime

class Alerts(db.Model):
    __tablename__ = 'Alerts'
    alert_id = db.Column(db.Integer, primary_key=True)
    alert_type = db.Column(db.String(100), nullable=False)
    severity = db.Column(db.Enum('LOW', 'MEDIUM', 'HIGH', 'CRITICAL'), default='MEDIUM', nullable=False)
    snail_observation_id = db.Column(db.Integer, db.ForeignKey('SnailObservations.observation_id'))
    water_quality_reading_id = db.Column(db.Integer, db.ForeignKey('WaterQualityReadings.reading_id'))
    latitude = db.Column(db.Numeric(10, 8), nullable=False)
    longitude = db.Column(db.Numeric(11, 8), nullable=False)
    alert_timestamp = db.Column(db.DateTime, nullable=False)
    description = db.Column(db.Text)
    status = db.Column(db.Enum('NEW', 'INVESTIGATING', 'ACTION_TAKEN', 'RESOLVED', 'FALSE_ALARM'), default='NEW', nullable=False)
    resolved_by_user_id = db.Column(db.Integer, db.ForeignKey('Users.user_id'))
    resolved_at = db.Column(db.DateTime)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        """
        将警报对象转换为字典，便于序列化为JSON
        """
        return {
            'alert_id': self.alert_id,
            'alert_type': self.alert_type,
            'severity': self.severity,
            'snail_observation_id': self.snail_observation_id,
            'water_quality_reading_id': self.water_quality_reading_id,
            'latitude': float(self.latitude),
            'longitude': float(self.longitude),
            'alert_timestamp': self.alert_timestamp,
            'description': self.description,
            'status': self.status,
            'resolved_by_user_id': self.resolved_by_user_id,
            'resolved_at': self.resolved_at,
            'created_at': self.created_at,
            'updated_at': self.updated_at
        }

