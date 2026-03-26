# -*- coding: utf-8 -*-
"""
Users表的ORM模型定义
"""
from models import db
from datetime import datetime
from werkzeug.security import check_password_hash, generate_password_hash


class Users(db.Model):
    __tablename__ = 'Users'
    user_id = db.Column(db.Integer, primary_key=True)
    email = db.Column(db.String(255), unique=True, nullable=False)
    password_hash = db.Column(db.String(255), nullable=False)
    full_name = db.Column(db.String(100))
    role = db.Column(db.String(50), default='admin')
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow)

    def set_password(self, password):
            """使用 werkzeug 生成密码哈希并存储"""
            self.password_hash = generate_password_hash(password)

    def check_password(self, password):
            """使用 werkzeug 检查提供的密码是否与存储的哈希匹配"""
            return check_password_hash(self.password_hash, password)

    def to_dict(self):
        """
        将用户对象转换为字典，便于序列化为JSON
        """
        return {
            'user_id': self.user_id,
            'email': self.email,
            'full_name': self.full_name,
            'role': self.role,
        }

