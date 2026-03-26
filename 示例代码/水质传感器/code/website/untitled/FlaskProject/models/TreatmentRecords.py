# -*- coding: utf-8 -*-
"""
TreatmentRecords表的ORM模型定义
"""
from models import db
from datetime import datetime

class TreatmentRecords(db.Model):
    __tablename__ = 'TreatmentRecords'
    treatment_record_id = db.Column(db.Integer, primary_key=True)
    treatment_date = db.Column(db.Date, nullable=False)
    count_before_treatment = db.Column(db.Integer, nullable=False)
    date_of_count_before = db.Column(db.Date, nullable=False)
    count_after_treatment = db.Column(db.Integer)
    date_of_count_after = db.Column(db.Date)
    efficacy_notes = db.Column(db.Text)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def to_dict(self):
        """
        将治理记录对象转换为字典，便于序列化为JSON
        """
        return {
            'treatment_record_id': self.treatment_record_id,
            'treatment_date': self.treatment_date,
            'count_before_treatment': self.count_before_treatment,
            'date_of_count_before': self.date_of_count_before,
            'count_after_treatment': self.count_after_treatment,
            'date_of_count_after': self.date_of_count_after,
            'efficacy_notes': self.efficacy_notes,
            'created_at': self.created_at,
            'updated_at': self.updated_at
        }

