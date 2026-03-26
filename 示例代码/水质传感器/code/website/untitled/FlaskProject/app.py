from flask import Flask
from models import db
from models.Users import Users
from models.Voyages import Voyages
from models.VoyagePathPoints import VoyagePathPoints
from models.WaterQualityReadings import WaterQualityReadings
from models.SnailObservations import SnailObservations
from models.TreatmentRecords import TreatmentRecords
from models.Alerts import Alerts
from flask_cors import CORS
from routes.users import users_bp
from routes.voyages import voyages_bp
from routes.voyage_path_points import vpp_bp
from routes.water_quality_readings import wq_bp
from routes.snail_observations import snail_bp
from routes.treatment_records import treat_bp
from routes.alerts import alerts_bp

app = Flask(__name__)
CORS(app)
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://root:Zwh119811@localhost/my_database'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db.init_app(app)

# 注册所有蓝图
app.register_blueprint(users_bp)
app.register_blueprint(voyages_bp)
app.register_blueprint(vpp_bp)
app.register_blueprint(wq_bp)
app.register_blueprint(snail_bp)
app.register_blueprint(treat_bp)
app.register_blueprint(alerts_bp)

# @app.route('/')
# def hello_world():
#     return 'Hello World!'

if __name__ == '__main__':
    app.run()
