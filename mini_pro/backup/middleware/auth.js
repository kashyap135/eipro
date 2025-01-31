const jwt = require('jsonwebtoken');

const JWT_SECRET = 'your-secret-key'; // In production, use an environment variable

function authenticateToken(req, res, next) {
    const authHeader = req.headers['authorization'];
    const token = authHeader && authHeader.split(' ')[1]; // Bearer TOKEN format

    if (!token) {
        return res.status(401).json({ message: 'Authentication token required' });
    }

    try {
        const user = jwt.verify(token, JWT_SECRET);
        req.user = user;
        next();
    } catch (err) {
        return res.status(403).json({ message: 'Invalid or expired token' });
    }
}

module.exports = {
    authenticateToken
};