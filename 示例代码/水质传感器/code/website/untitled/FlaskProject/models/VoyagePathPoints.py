# -*- coding: utf-8 -*-
"""
VoyagePathPoints表的ORM模型定义
"""
from models import db
from datetime import datetime

class VoyagePathPoints(db.Model):
    __tablename__ = 'VoyagePathPoints'
    point_id = db.Column(db.BigInteger, primary_key=True)
    voyage_id = db.Column(db.Integer, db.ForeignKey('Voyages.voyage_id'), nullable=False)
    latitude = db.Column(db.Numeric(10, 8), nullable=False)
    longitude = db.Column(db.Numeric(11, 8), nullable=False)
    altitude = db.Column(db.Numeric(7, 2))
    speed = db.Column(db.Numeric(5, 2))
    timestamp = db.Column(db.DateTime, nullable=False)

    def to_dict(self):
        """
        将路径点对象转换为字典，便于序列化为JSON
        """
        return {
            'point_id': self.point_id,
            'voyage_id': self.voyage_id,
            'latitude': float(self.latitude),
            'longitude': float(self.longitude),
            'altitude': float(self.altitude) if self.altitude is not None else None,
            'speed': float(self.speed) if self.speed is not None else None,
            'timestamp': self.timestamp
        }

