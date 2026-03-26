# -*- coding: utf-8 -*-
"""
Voyages表的ORM模型定义
"""
from models import db
from datetime import datetime

class Voyages(db.Model):
    __tablename__ = 'Voyages'
    voyage_id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('Users.user_id'))
    vessel_identifier = db.Column(db.String(100))
    start_timestamp = db.Column(db.DateTime, nullable=False)
    end_timestamp = db.Column(db.DateTime)
    purpose = db.Column(db.Text)
    notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        """
        将航行记录对象转换为字典，便于序列化为JSON
        """
        return {
            'voyage_id': self.voyage_id,
            'user_id': self.user_id,
            'vessel_identifier': self.vessel_identifier,
            'start_timestamp': self.start_timestamp,
            'end_timestamp': self.end_timestamp,
            'purpose': self.purpose,
            'notes': self.notes,
            'created_at': self.created_at,
            'updated_at': self.updated_at
        }

